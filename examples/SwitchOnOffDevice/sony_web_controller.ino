#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>


#include "ESP8266_Slink.h"

#ifndef STASSID
#define STASSID "SSID"
#define STAPSK  "PSK"
#endif

#define CTRL_A1_PIN 12 // Control-A1 pin

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);
Slink slink;

const int led = 13;

/**
 * @brief Handles building the landing root page. Provides curl examples
 * 
 */
void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "Welcome to your Sony Amp Controller!\n\nTry: \n\t curl -4 -v"
  " \"http://stereo.local/update?type=power&value=on\""
  "\nOr: \n\t curl -4 -v \"http://stereo.local/update?type=volume&value=up\""
  "\nOr: \n\t curl -4 -v \"http://stereo.local/update?type=mute&value=on\""
  "\nOr: \n\t curl -4 -v \"http://stereo.local/update?type=input&value=phono\"");
  digitalWrite(led, 0);
}

/**
 * @brief Handles bad requests
 * 
 */
void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("stereo")) {
    Serial.println("MDNS responder started");
  }
  slink.init(CTRL_A1_PIN); // Set-up S-Link pin
  server.on("/", handleRoot);
  
server.on("/update", HTTP_GET, []() {
    String type = server.arg("type");;
    String value = server.arg("value");;
    // GET input1 value on <ESP_IP>/update?type=<type>&value=<value>
    if ((type != "") && (value != "")) {
      if (type == "power") {
        unsigned int state = (value.equals("on"))? SLINK_CMD_AMP_POWER_ON : SLINK_CMD_AMP_POWER_OFF;
        slink.sendCommand(SLINK_DEVICE_AMP, state);
      }
      else if (type == "mute") {
        unsigned int state = (value.equals("on"))? SLINK_CMD_AMP_MUTE_ON : SLINK_CMD_AMP_MUTE_OFF;
        slink.sendCommand(SLINK_DEVICE_AMP, state);
      }
      else if (type == "volume") {
        if (value.equals("up")) {
          slink.sendCommand(SLINK_DEVICE_AMP, SLINK_CMD_AMP_VOLUME_UP);
        }
        else if (value.equals("down")) {
          slink.sendCommand(SLINK_DEVICE_AMP, SLINK_CMD_AMP_VOLUME_DOWN);
        }
      }
      else if (type == "input") {
        unsigned int input_val;
        if (value == "tuner") input_val = SLINK_CMDP_IN_TUNER;
        else if (value == "phono") input_val = SLINK_CMDP_IN_PHONO;
        else if (value == "cd") input_val = SLINK_CMDP_IN_CD;
        else if (value == "dat") input_val = SLINK_CMDP_IN_DAT;
        else input_val = SLINK_CMDP_IN_MD;
        slink.sendCommand(SLINK_DEVICE_AMP, SLINK_CMD_AMP_SET_INPUT_CHAN, input_val);
      }
    }
    else {
      type = "No message sent";
      value = "No message sent";
    }
    Serial.print("type: ");
    Serial.print(type);
    Serial.print(" - Set to: ");
    Serial.println(value);
    server.send(200, "text/plain", "OK");
  });
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
}