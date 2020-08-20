# ESP8266_Slink

**ESP8266 SONY S-LINK/Control-A1 Library.**

This library allows control of Sony Home Entertainment Devices via Sony S-link/Control-A1/Control A-1 II.

(C) Tyler Goulding, 2020, CC-BY-SA 4.0
https://creativecommons.org/licenses/by-sa/4.0/

The library is designed specifically for an ESP8266. I may provide support for other devices if needed. The
library makes uses of hw timer 0 so do not use take control of the timer for your own needs.

For hardware, make sure to connect the slink line via a 220R and preferably a diode. I have been running it without a diode simply because I had no free ones when I made this library. If you are only controlling a single device, then this should be fine. If you are driving a full home entertainment system, use a diode to limit current in the case of collisions.

This library is based off Ircama's Sony SLink library:
    https://github.com/Ircama/Sony_SLink.git
-------------------------------------------------------------------------------------------------------------------

# Installation

Go to the libraries directory in your Arduino folder. and clone this repo

```bash
$ cd path/to/Arduino/libraries
$ git clone https://github.com/tylerGoulding/ESP8266_Slink.git
```

# Usage
To use the library in your own sketch, select it from *Sketch > Import Library*.

# Example

Simplest example of using this library:

```c++
#define CTRL_A1_PIN 2 // S-Link Control-A1 pin

#include "ESP8266_Slink.h"
Slink slink;

void setup() {
  slink.init(CTRL_A1_PIN); // Set-up S-Link pin
  slink.sendCommand(SLINK_DEVICE_AMP, SLINK_CMD_AMP_POWER_ON);
}

void loop() {
}
```

Check the *examples* directory for further examples.

# Description of the functions

`slink.init(slinkPin)` - set-up S-Link pin

`slink.sendCommand(device, command)` - send a command to the device;

# Notes

  Special thanks to Ircama. His code provided the timing and protocol.
  I simply made it timer based and work on an ESP8266.

  Code heavily based on
  - Ircama's code:
    https://github.com/Ircama/Sony_SLink.git

  Ircama's code heavily based on:
  - hajdbo's code:
    https://github.com/hajdbo/sony-jukebox-slink/blob/master/jukebox.pde
  - EdsterG's code:
    https://github.com/EdsterG/LANC_Library

  The following are reference documents compiled by Ircama as well:
  - Sony S-Link gateway document:
    http://ee.bradley.edu/projects/proj2001/sonyntwk/SLink.PDF

  - How SONY's S-LINK/CTRL-A(II) protocol works (waveform):
    http://boehmel.de/slink.htm

  - Codes for Sony STR-DA50ES receiver:
    http://www.hifi-remote.com/sony/Sony_dsp_192.htm

  - S-Link Parallel Device for Linux
    http://web.archive.org/web/20040722060841/http://www.undeadscientist.com/slink/index.html

  - Reverse Engineered Control-A1 codes (Using the Sony CDP-CX250) Written by BigDave (2/19/98)
    http://web.archive.org/web/19981202175957/http://www.cc.gatech.edu/people/home/bigdave/cdplayer/control-a1.txt

  - Working S-Link code - Background of the S-link protocol
    http://forums.parallax.com/discussion/70973/working-s-link-code-long

  - Slink Send and Receive Commands for Sony MiniDisc Recorders
    http://web.archive.org/web/20030419100153/http://web2.airmail.net/will/mdslink.txt

  - Jukebox Interface Guide
    http://web.archive.org/web/20030414200645/http://www.upl.cs.wisc.edu/~orn/jukebox/guide.html

  - Original Control-A1 document
    http://web.archive.org/web/20030414231523/http://www.upl.cs.wisc.edu/~orn/jukebox/controla1.html

  Tested with a Sony STR-DA50ES receiver/amplifier.
  Service manual: http://sportsbil.com/sony/STR/STR-D/STR-DA30ES_DA50ES_V55ES_v1.1.pdf

  Feel free to share this source code, but include explicit mention to the author.
  Licensed under creative commons - see http://creativecommons.org/licenses/by-sa/4.0/
