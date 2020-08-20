/**
 ****************************************************************************************
 * @file ESP8266_Slink.cpp
 *
 * @brief Arduino SONY Interupt driven Control-A1 Protocol Library
 *
 * Code heavily based on
 *   - Ircama's code:
 *     https://github.com/Ircama/Sony_SLink.git
 ****************************************************************************************
 */
#include "ESP8266_Slink.h"

/*
 * DEFINES
 ****************************************************************************************
 */
// DEBUG - set to 1 for debug prints. This may mess up timing.
#define ESP8266_SLINK_DEBUG 0

/// Max count the ESP8266 can handle for timer1.
#define MAX_ESP8266_TIMER_COUNT 8388607

/*
 * TYPEDEFS
 ****************************************************************************************
 */
typedef void (*timer_callback)(void);

/*
 * VARIABLES
 ****************************************************************************************
 */
/// this is the write array that stores the active command
volatile static uint8_t write_arr[4];
/// current array index of active write
volatile static uint8_t write_arr_idx;
/// current byte index of active write
volatile static int8_t write_byte_idx;
/// size of the command.
volatile static int8_t write_size;
/// slink pin
volatile static int slink_pin;

/*
 * LOCAL FUNCTIONS
 ****************************************************************************************
 */
void handle_send_command();

/**
 * @brief enable a single shot timer to a specified callback
 *
 * @param[in] cycles   - number of cycles for timer. Must be less than
 * MAX_ESP8266_TIMER_COUNT
 * @param[in] callback - callback when timer interrupt fires
 * @return true if enabled successfully
 * @return false otherwise
 */
static bool enableSingleShotTimer(uint32_t cycles, timer_callback callback) {
  if (cycles > MAX_ESP8266_TIMER_COUNT) {
    return false;
  }
#if (ESP8266_SLINK_DEBUG)
  Serial.println("ESP8266_Slink: prog timer, count = " + String(_timerCount));
#endif //(ESP8266_SLINK_DEBUG)

  timer1_attachInterrupt(callback);
  timer1_write(cycles);
  // Interrupt on EDGE, single
  timer1_enable(TIM_DIV1, TIM_EDGE, TIM_SINGLE);
  return true;
}

/**
 * @brief Checks to see if the line is free.
 *
 */
static void ICACHE_RAM_ATTR line_check() {
#if (ESP8266_SLINK_DEBUG)
  Serial.println("ESP8266_Slink: line check.");
#endif //(ESP8266_SLINK_DEBUG)
  static bool start_line_check = true;
  static unsigned long Start;
  static unsigned long beginTimeout;
  if (start_line_check) {
    Start = micros();
    beginTimeout = Start;
    start_line_check = false;
  }
  unsigned long curr = micros();
  if (curr - Start > SLINK_LINE_READY_uS) {
    start_line_check = true;
    handle_send_command();
    return;
  } else if (curr - beginTimeout > SLINK_LOOP_TIMEOUT_uS) {
    handle_send_command();
    return;
  } else {
    enableSingleShotTimer(SLINK_LOOP_DELAY_CYCLES, lineCheck);
  }
}

/**
 * @brief Write sync to take control of the line by holding the line low for
 * SLINK_MARK_SYNC_uS microseconds
 *
 */
static void ICACHE_RAM_ATTR write_sync() {
#if (ESP8266_SLINK_DEBUG)
  Serial.print("write_sync: ");
  Serial.println(micros());
#endif //(ESP8266_SLINK_DEBUG)
  static bool sync_mark_delimiter = false;
  if (sync_mark_delimiter) {
    digitalWrite(slink_pin, HIGH); // release the default HIGH state
    sync_mark_delimiter = false;
    enableSingleShotTimer(SLINK_MARK_DELIMITER_CYCLES, handle_send_command);
  } else {
    digitalWrite(slink_pin, LOW); // start sync (line low)
    sync_mark_delimiter = true;
    enableSingleShotTimer(SLINK_MARK_SYNC_CYCLES, write_sync);
  }
}

/**
 * @brief write bytes for the current command.
 */
static void ICACHE_RAM_ATTR write_bytes() {
  static bool mark_delimiter = false;
  if (write_arr_idx < write_size) {
    uint8_t write_byte = write_arr[write_arr_idx];
    if (mark_delimiter) {
      if (write_byte_idx == 0) {
        write_arr_idx += 1;
        write_byte_idx = 7;
      } else {
        write_byte_idx -= 1;
      }
      mark_delimiter = false;
      digitalWrite(slink_pin, HIGH);
      enableSingleShotTimer(SLINK_MARK_DELIMITER_CYCLES, write_bytes);
    } else {
      bool isOne = (write_byte & (1 << write_byte_idx));
      mark_delimiter = true;
      digitalWrite(slink_pin, LOW);
      Serial.println(isOne);
      enableSingleShotTimer((isOne) ? SLINK_MARK_ONE_CYCLES
                                    : SLINK_MARK_ZERO_CYCLES,
                            write_bytes);
    }
  } else {
    write_byte_idx = 7;
    mark_delimiter = false;
    digitalWrite(slink_pin, HIGH);
    handle_send_command();
    return;
  }
}

/**
 * @brief handles sending the current active command.
 *
 */
static void ICACHE_RAM_ATTR handle_send_command() {
  static bool write_sync = true;
  static bool line_check = true;
  // if we are at the end of our message, break.
  if (write_arr_idx >= write_size) {
#if (ESP8266_SLINK_DEBUG)
    Serial.print("DONE: ");
    Serial.println(micros());
#endif //(ESP8266_SLINK_DEBUG)
    pinMode(slink_pin, INPUT);
    timer1_disable();
    write_sync = true;
    line_check = true;
    return;
  } else if (line_check) {
#if (ESP8266_SLINK_DEBUG)
    Serial.println("ESP8266_Slink:  ----- ----- lineCheck");
#endif
    pinMode(slink_pin, INPUT);
    line_check = false;
    line_check();
  } else if (write_sync) {
#if (ESP8266_SLINK_DEBUG)
    Serial.println("ESP8266_Slink:  ----- ----- write_sync");
#endif //(ESP8266_SLINK_DEBUG)
    write_sync = false;
    pinMode(slink_pin, OUTPUT);
    write_sync();
  } else if (write_arr_idx < write_size) {
#if (ESP8266_SLINK_DEBUG)
    Serial.println("ESP8266_Slink:  ----- ----- write_bytes");
#endif //(ESP8266_SLINK_DEBUG)
    pinMode(slink_pin, OUTPUT);
    write_bytes();
  } else {
    // TODO: need to pad this to meet length of message... should be 45us
    //       pre-compute to needed delay to make faster
  }
}

/* Initialize protocol */

void Slink::init(int slinkPin) {
  slink_pin = slinkPin;
  // Default to INPUT will drive as output when needed
  pinMode(slink_pin, INPUT);
}

void Slink::sendCommand(uint8_t deviceId, uint8_t commandId1, int commandId2,
                        int commandId3) {
  // TODO: take array + size.. some commands can be longer than 4 bytes.
  // TODO: check if  command in transmition.
  // TODO: double buffer to store next command.
  write_arr[0] = (volatile uint8_t)deviceId;
  write_arr[1] = (volatile uint8_t)commandId1;
#if (ESP8266_SLINK_DEBUG)
  Serial.print("Send command: [");
  Serial.print((uint8_t)write_arr[0]);
  Serial.print(", ");
  Serial.print((uint8_t)write_arr[1]);
#endif //(ESP8266_SLINK_DEBUG)
  write_size = 2;
  if (commandId2 >= 0) {
    write_size = 3;
    write_arr[2] = commandId2;
#if (ESP8266_SLINK_DEBUG)
    Serial.print(", ");
    Serial.print((uint8_t)write_arr[2]);
#endif //(ESP8266_SLINK_DEBUG)
  }
  if (commandId3 >= 0) {
    write_size = 4;
    write_arr[3] = commandId2;
#if (ESP8266_SLINK_DEBUG)
    Serial.print(", ");
    Serial.print((uint8_t)write_arr[3]);
#endif //(ESP8266_SLINK_DEBUG)
  }
#if (ESP8266_SLINK_DEBUG)
  Serial.print("]; ");
  Serial.println(micros());
#endif //(ESP8266_SLINK_DEBUG)
  write_arr_idx = 0;
  write_byte_idx = 7;
  handle_send_command();
}
