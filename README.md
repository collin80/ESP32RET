ESP32RET
=======

Reverse Engineering Tool running on EVTV ESP32Due Hardware

A fork of the M2RET project, itself a fork of the GVRET project.

#### Requirements:

You will need the following to be able to compile the run this project:

- [Arduino IDE](https://www.arduino.cc/en/Main/Software) Tested on 1.8.4
- [Arduino-ESP32](https://github.com/espressif/arduino-esp32) - Allows for programming the ESP32 with the Arduino IDE
- [esp32_can](https://github.com/collin80/esp32_can) - A unified CAN library that supports the built-in CAN plus MCP2515 and MCP2517FD

All libraries belong in %USERPROFILE%\Documents\Arduino\hardware\esp32\libraries (Windows) or ~/Arduino/hardware/esp32/libraries (Linux/Mac).

The canbus is supposed to be terminated on both ends of the bus. This should not be a problem as this firmware will be used to reverse engineer existing buses. However, do note that CAN buses should have a resistance from CAN_H to CAN_L of 60 ohms. This is affected by placing a 120 ohm resistor on both sides of the bus. If the bus resistance is not fairly close to 60 ohms then you may run into trouble.  

#### The firmware is a work in progress. What works:
- CAN0 and CAN1 are operational
- EEPROM can be used to save settings between start ups
- Text console is active (configuration and CAN capture display)
- Can connect as a GVRET device with SavvyCAN
- LAWICEL support (somewhat tested. Still experimental)

#### What does not work:
- Digital and Analog I/O

#### License:

This software is MIT licensed:

Copyright (c) 2014-2018 Collin Kidder, Michael Neuweiler, Charles Galpin

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

