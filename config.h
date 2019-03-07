/*
 * config.h
 *
 * allows the user to configure static parameters.
 *
 * Note: Make sure with all pin defintions of your hardware that each pin number is
 *       only defined once.

 Copyright (c) 2013-2018 Collin Kidder, Michael Neuweiler, Charles Galpin

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
 *      Author: Michael Neuweiler
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "esp32_can.h"
#include <WiFi.h>

//buffer size for SDCard - Sending canbus data to the card.
//This is a large buffer but the sketch may as well use up a lot of RAM. It's there.
//This value is picked up by the SD card library and not directly used in the GVRET code.
#define BUF_SIZE    512

//size to use for buffering writes to the USB bulk endpoint
//This is, however, directly used.
#define SER_BUFF_SIZE       512

//Buffer for CAN frames when sending over wifi. This allows us to build up a multi-frame packet that goes
//over the air all at once. This is much more efficient than trying to send a new TCP/IP packet for each and every
//frame. It delays frames from getting to the other side a bit but that's life.
#define WIFI_BUFF_SIZE      2048

//Number of microseconds between hard flushes of the serial buffer (if not in wifi mode) or the wifi buffer (if in wifi mode)
//This keeps the latency more consistent. Otherwise the buffer could partially fill and never send.
#define SER_BUFF_FLUSH_INTERVAL 50000

#define CFG_BUILD_NUM   362
#define CFG_VERSION "ESP32RET Beta Mar 06 2019"
#define EEPROM_VER      0x21

#define MARK_LIMIT  6   //# of our analog input pins to use for marking. Defaults to all of them. Send voltage to pin to trigger it

#define NUM_ANALOG  NUM_ANALOG_INPUTS   // we get the number of analogue inputs from variant.h
#define NUM_DIGITAL 6   // Not currently using digital pins on the ESP32
#define NUM_OUTPUT  6   // Ditto

#define NUM_BUSES   2   //number of buses possible on this hardware - CAN0, CAN1

//It's not even used on this hardware currently. But, slows down the blinks to make them more visible
#define BLINK_SLOWNESS  100 

//How many devices to allow to connect to our WiFi telnet port?
#define MAX_CLIENTS 1

struct FILTER {  //should be 10 bytes
    uint32_t id;
    uint32_t mask;
    boolean extended;
    boolean enabled;
};

enum FILEOUTPUTTYPE {
    NONE = 0,
    BINARYFILE = 1,
    GVRET = 2,
    CRTD = 3
};

struct EEPROMSettings {
    uint8_t version;

    uint32_t CAN0Speed;
    uint32_t CAN1Speed;
    uint32_t CAN1FDSpeed;
    boolean CAN0_Enabled;
    boolean CAN1_Enabled;
    boolean CAN1_FDMode;

    boolean useBinarySerialComm; //use a binary protocol on the serial link or human readable format?
    FILEOUTPUTTYPE fileOutputType; //what format should we use for file output?

    char fileNameBase[30]; //Base filename to use
    char fileNameExt[4]; //extension to use
    uint16_t fileNum; //incrementing value to append to filename if we create a new file each time
    boolean appendFile; //start a new file every power up or append to current?
    boolean autoStartLogging; //should logging start immediately on start up?

    uint8_t logLevel; //Level of logging to output on serial line
    uint8_t sysType; 

    uint16_t valid; //stores a validity token to make sure EEPROM is not corrupt

    boolean CAN0ListenOnly; //if true we don't allow any messing with the bus but rather just passively monitor.
    boolean CAN1ListenOnly;

    //if we're using WiFi then output to serial is disabled (it's far too slow to keep up)  
    uint8_t wifiMode; //0 = don't use wifi, 1 = connect to an AP, 2 = Create an AP
    uint8_t SSID[32];     //null terminated string for the SSID
    uint8_t WPA2Key[64]; //Null terminated string for the key. Can be a passphase or the actual key
};

struct SystemSettings {
    boolean useSD; //should we attempt to use the SDCard? (No logging possible otherwise)
    boolean logToFile; //are we currently supposed to be logging to file?
    boolean SDCardInserted;
    uint8_t LED_CANTX;
    uint8_t LED_CANRX;
    uint8_t LED_LOGGING;
    boolean txToggle; //LED toggle values
    boolean rxToggle;
    boolean logToggle;
    boolean lawicelMode;
    boolean lawicellExtendedMode;
    boolean lawicelAutoPoll;
    boolean lawicelTimestamping;
    int lawicelPollCounter;
    boolean lawicelBusReception[NUM_BUSES]; //does user want to see messages from this bus?
    int8_t numBuses; //number of buses this hardware currently supports.
    WiFiClient clientNodes[MAX_CLIENTS];
    boolean isWifiConnected;
    boolean isWifiActive;
};

extern EEPROMSettings settings;
extern SystemSettings SysSettings;

#endif /* CONFIG_H_ */
