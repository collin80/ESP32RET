/*
 *  ELM327_Emu.cpp
 *
 * Class emulates the serial comm of an ELM327 chip - Used to create an OBDII interface
 *
 * Created: 3/23/2017
 *  Author: Collin Kidder
 */

/*
 Copyright (c) 2017 Collin Kidder

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
 */

#include "ELM327_Emulator.h"

/*
 * Constructor. Assign serial interface to use for comm with bluetooth adapter we're emulating with
 */
ELM327Emu::ELM327Emu() {
    serialInterface = &Serial;
}

/*
 * Constructor. Pass serial interface to use
 */
ELM327Emu::ELM327Emu(HardwareSerial *which) {
    serialInterface = which;
}

/*
 * Initialization of hardware and parameters
 */
void ELM327Emu::setup() {

    tickCounter = 0;
    ibWritePtr = 0;
    serialInterface->begin(115200);
}

/*
 * Send a command to ichip. The "AT+i" part will be added.
 */
void ELM327Emu::sendCmd(String cmd) {
    serialInterface->write("AT");
    serialInterface->print(cmd);
    serialInterface->write(13);
    loop(); // parse the response
}

/*
 * Called in the main loop (hopefully) in order to process serial input waiting for us
 * from the wifi module. It should always terminate its answers with 13 so buffer
 * until we get 13 (CR) and then process it.
 * But, for now just echo stuff to our serial port for debugging
 */

void ELM327Emu::loop() {
    int incoming;
    while (serialInterface->available()) {
        incoming = serialInterface->read();
        if (incoming != -1) { //and there is no reason it should be -1
            if (incoming == 13 || ibWritePtr > 126) { // on CR or full buffer, process the line
                incomingBuffer[ibWritePtr] = 0; //null terminate the string
                ibWritePtr = 0; //reset the write pointer

                if (Logger::isDebug())
                    Logger::debug(incomingBuffer);
                processCmd();

            } else { // add more characters
                if (incoming != 10 && incoming != ' ') // don't add a LF character or spaces. Strip them right out
                    incomingBuffer[ibWritePtr++] = (char)tolower(incoming); //force lowercase to make processing easier
            }
        } else
            return;
    }
}

/*
*   There is no need to pass the string in here because it is local to the class so this function can grab it by default
*   But, for reference, this cmd processes the command in incomingBuffer
*/
void ELM327Emu::processCmd() {
    String retString = processELMCmd(incomingBuffer);

    serialInterface->print(retString);
    if (Logger::isDebug()) {
        char buff[30];
        retString.toCharArray(buff, 30);
        Logger::debug(buff);
    }

}

String ELM327Emu::processELMCmd(char *cmd) {
    String retString = String();
    String lineEnding;
    if (bLineFeed) lineEnding = String("\r\n");
    else lineEnding = String("\r");

    if (!strncmp(cmd, "at", 2)) {

        if (!strcmp(cmd, "atz")) { //reset hardware
            retString.concat(lineEnding);
            retString.concat("ELM327 v1.3a");
        }
        else if (!strncmp(cmd, "atsh",4)) { //set header address
            //ignore this - just say OK
            retString.concat("OK");
        }
        else if (!strncmp(cmd, "ate",3)) { //turn echo on/off
            //could support echo but I don't see the need, just ignore this
            retString.concat("OK");
        }
        else if (!strncmp(cmd, "ath",3)) { //turn headers on/off
            if (cmd[3] == '1') bHeader = true;
            else bHeader = false;
            retString.concat("OK");
        }
        else if (!strncmp(cmd, "atl",3)) { //turn linefeeds on/off
            if (cmd[3] == '1') bLineFeed = true;
            else bLineFeed = false;
            retString.concat("OK");
        }
        else if (!strcmp(cmd, "at@1")) { //send device description
            retString.concat("ELM327 Emulator");
        }
        else if (!strcmp(cmd, "ati")) { //send chip ID
            retString.concat("ELM327 v1.3a");
        }
        else if (!strncmp(cmd, "atat",4)) { //set adaptive timing
            //don't intend to support adaptive timing at all
            retString.concat("OK");
        }
        else if (!strncmp(cmd, "atsp",4)) { //set protocol
            //theoretically we can ignore this
            retString.concat("OK");
        }
        else if (!strcmp(cmd, "atdp")) { //show description of protocol
            retString.concat("can11/500");
        }
        else if (!strcmp(cmd, "atdpn")) { //show protocol number (same as passed to sp)
            retString.concat("6");
        }
        else if (!strcmp(cmd, "atd")) { //set to defaults
            retString.concat("OK");
        }
        else if (!strncmp(cmd, "atm", 3)) { //turn memory on/off
            retString.concat("OK");
        }
        else if (!strcmp(cmd, "atrv")) { //show 12v rail voltage
            //TODO: the system should actually have this value so it wouldn't hurt to
            //look it up and report the real value.
            retString.concat("14.2V");
        }
        else { //by default respond to anything not specifically handled by just saying OK and pretending.
            retString.concat("OK");
        }
    }
    else { //if no AT then assume it is a PID request. This takes the form of four bytes which form the alpha hex digit encoding for two bytes
        //there should be four or six characters here forming the ascii representation of the PID request. Easiest for now is to turn the ascii into
        //a 16 bit number and mask off to get the bytes
        if (strlen(cmd) == 4) {
            uint32_t valu = strtol((char *) cmd, NULL, 16); //the pid format is always in hex
            uint8_t pidnum = (uint8_t)(valu & 0xFF);
            uint8_t mode = (uint8_t)((valu >> 8) & 0xFF);
            Logger::debug("Mode: %i, PID: %i", mode, pidnum);
            char out[7];
            char buff[10];
            if (processRequest(mode, pidnum, NULL, out)) {
                if (bHeader) {
                    retString.concat("7E8");
                    out[0] += 2; //not sending only data bits but mode and pid too
                    for (int i = 0; i <= out[0]; i++) {
                        sprintf(buff, "%02X", out[i]);
                        retString.concat(buff);
                    }
                }
                else {
                    mode += 0x40;
                    sprintf(buff, "%02X", mode);
                    retString.concat(buff);
                    sprintf(buff, "%02X", pidnum);
                    retString.concat(buff);
                    for (int i = 1; i <= out[0]; i++) {
                        sprintf(buff, "%02X", out[i+2]);
                        retString.concat(buff);
                    }
                }
            }
        }
    }

    retString.concat(lineEnding);
    retString.concat(">"); //prompt to show we're ready to receive again

    return retString;
}

/*
Public method to process OBD2 requests.
    inData is whatever payload the request might need to have sent - it's OK to be NULL if this is a run of the mill PID request with no payload
    outData should be a preallocated buffer of at least 6 bytes. The format is as follows:
    outData[0] is the length of the data actually returned
    outData[1] is the returned mode (input mode + 0x40)
    there after, the rest of the bytes are the data requested. This should be 1-5 bytes
*/
bool ELM327Emu::processRequest(uint8_t mode, uint8_t pid, char *inData, char *outData) {
    bool ret = false;
    switch (mode) {
    case 1: //show current data
        ret = processShowData(pid, inData, outData);
        outData[1] = mode + 0x40;
        outData[2] = pid;
        break;
    case 2: //show freeze frame data - not sure we'll be supporting this
        break;
    case 3: //show stored diagnostic codes - Returns DTC codes
        break;
    case 4: //clear diagnostic trouble codes - If we get this frame we just clear all codes no questions asked.
        break;
    case 6: //test results over CANBus (this replaces mode 5 from non-canbus) - I know nothing of this
        break;
    case 7: //show pending diag codes (current or last driving cycle) - Might just overlap with mode 3
        break;
    case 8: //control operation of on-board systems - this sounds really proprietary and dangerous. Maybe ignore this?
        break;
    case 9: //request vehicle info - We can identify ourselves here but little else
        break;
    case 0x20: //0x20 through something like 0x2F are custom PIDs. You make up what you want here.
        break;
    }
    return ret;
}

//Process SAE standard PID requests. Function returns whether it handled the request or not.
bool ELM327Emu::processShowData(uint8_t pid, char *inData, char *outData) {
    int temp;


    switch (pid) {
    case 0: //pids 1-0x20 that we support - bitfield
        //returns 4 bytes so immediately indicate that.
        outData[0] = 4;
        outData[3] = 0b11011000; //pids 1 - 8 - starting with pid 1 in the MSB and going from there
        outData[4] = 0b00010000; //pids 9 - 0x10
        outData[5] = 0b10000000; //pids 0x11 - 0x18
        outData[6] = 0b00010011; //pids 0x19 - 0x20
        return true;
        break;
    case 1: //Returns 32 bits but we really can only support the first byte which has bit 7 = Malfunction? Bits 0-6 = # of DTCs
        outData[0] = 4;
        outData[3] = 0; //Fault codes would go here if any were returned by the car
        outData[4] = 0; 
        outData[5] = 0; 
        outData[6] = 0;
        return true;
        break;
    case 2: //Freeze DTC
        return false; //don't support freeze framing yet. Might be useful in the future.
        break;
    case 4: //Calculated engine load (A * 100 / 255) - Percentage
        outData[0] = 1;
        outData[3] = 0xE4; //return a load of 89.4%
        return true;
        break;
    case 5: //Engine Coolant Temp (A - 40) = Degrees Centigrade
        outData[0] = 1; //returning only one byte
        outData[3] = 120; //Return 80C as the coolant temperature.
        return true;
        break;
    case 0xC: //Engine RPM (A * 256 + B) / 4
        temp = 4200; //1050 RPM
        outData[0] = 2;
        outData[3] = (uint8_t)(temp / 256);
        outData[4] = (uint8_t)(temp);
        return true;
        break;
    case 0x11: //Throttle position (A * 100 / 255) - Percentage
        outData[0] = 1;
        outData[3] = 127; //50% throttle
        return true;
        break;
    case 0x1C: //Standard supported (We return 1 = OBDII)
        outData[0] = 1;
        outData[3] = 1;
        return true;
        break;
    case 0x1F: //runtime since engine start (A*256 + B)
        temp = millis() / 1000; //return the actual board uptime
        outData[0] = 2;
        outData[3] = (uint8_t)(temp / 256);
        outData[4] = (uint8_t)(temp);
        return true;
        break;
    case 0x20: //pids supported (next 32 pids - formatted just like PID 0)
        outData[0] = 4;
        outData[3] = 0b10000000; //pids 0x21 - 0x28 - starting with pid 0x21 in the MSB and going from there
        outData[4] = 0b00000010; //pids 0x29 - 0x30
        outData[5] = 0b00000000; //pids 0x31 - 0x38
        outData[6] = 0b00000001; //pids 0x39 - 0x40
        return true;
        break;
    case 0x21: //Distance traveled with fault light lit (A*256 + B) - In km
        outData[0] = 2;
        outData[3] = 0; //there are no faults so no distance here either
        outData[4] = 0;
        return true;
        break;
    case 0x2F: //Fuel level (A * 100 / 255) - Percentage
        outData[0] = 1;
        outData[3] = 64; //a quarter tank of fuel
        return true;
        break;
    case 0x40: //PIDs supported, next 32
        outData[0] = 4;
        outData[3] = 0b00000000; //pids 0x41 - 0x48 - starting with pid 0x41 in the MSB and going from there
        outData[4] = 0b00000000; //pids 0x49 - 0x50
        outData[5] = 0b10000000; //pids 0x51 - 0x58
        outData[6] = 0b00000001; //pids 0x59 - 0x60
        return true;
        break;
    case 0x51: //What type of fuel do we use?
        outData[0] = 1;
        outData[3] = 8; //code came from an electric car ECU so say we're using electricity for fuel
        return true;
        break;
    case 0x60: //PIDs supported, next 32
        outData[0] = 4;
        outData[3] = 0b11100000; //pids 0x61 - 0x68 - starting with pid 0x61 in the MSB and going from there
        outData[4] = 0b00000000; //pids 0x69 - 0x70
        outData[5] = 0b00000000; //pids 0x71 - 0x78
        outData[6] = 0b00000000; //pids 0x79 - 0x80
        return true;
        break;
    case 0x61: //Driver requested torque (A-125) - Percentage
        outData[0] = 1;
        outData[3] = 175; //50% torque requested
        return true;
        break;
    case 0x62: //Actual Torque delivered (A-125) - Percentage
        outData[0] = 1;
        outData[3] = 168; //getting less torque than requested.
        return true;
        break;
    case 0x63: //Reference torque for engine - presumably max torque - A*256 + B -> In Nm
        temp = 600; //Powerful!
        outData[0] = 2;
        outData[3] = (uint8_t)(temp / 256);
        outData[4] = (uint8_t)(temp & 0xFF);
        return true;
        break;
    }
    return false;
}

//Add any custom PIDs you want in here. Format code as above ^^^ 0 = Returned data length, 3-? = Data returned.
bool ELM327Emu::processShowCustomData(uint16_t pid, char *inData, char *outData) {
    switch (pid) {
    }
    return false;
}
