/*
Implements handling of the GVRET comm protocol, both sending and receiving
*/

#include "gvret_comm.h"
#include "SerialConsole.h"
#include "config.h"
#include "can_manager.h"

GVRET_Comm_Handler::GVRET_Comm_Handler()
{
    step = 0;
    state = IDLE;
    transmitBufferLength = 0;
}

void GVRET_Comm_Handler::processIncomingByte(uint8_t in_byte)
{
    uint32_t busSpeed = 0;
    uint32_t now = micros();

    uint8_t temp8;
    uint16_t temp16;

    switch (state) {
    case IDLE:
        if(in_byte == 0xF1)
        {
            state = GET_COMMAND;
        }
        else if(in_byte == 0xE7)
        {
            settings.useBinarySerialComm = true;
            SysSettings.lawicelMode = false;
            //setPromiscuousMode(); //going into binary comm will set promisc. mode too.
        } 
        else
        {
            console.rcvCharacter((uint8_t) in_byte);
        }
        break;
    case GET_COMMAND:
        switch(in_byte)
        {
        case PROTO_BUILD_CAN_FRAME:
            state = BUILD_CAN_FRAME;
            buff[0] = 0xF1;
            step = 0;
            break;
        case PROTO_TIME_SYNC:
            state = TIME_SYNC;
            step = 0;
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 1; //time sync
            transmitBuffer[transmitBufferLength++] = (uint8_t) (now & 0xFF);
            transmitBuffer[transmitBufferLength++] = (uint8_t) (now >> 8);
            transmitBuffer[transmitBufferLength++] = (uint8_t) (now >> 16);
            transmitBuffer[transmitBufferLength++] = (uint8_t) (now >> 24);
            break;
        case PROTO_DIG_INPUTS:
            //immediately return the data for digital inputs
            temp8 = 0; //getDigital(0) + (getDigital(1) << 1) + (getDigital(2) << 2) + (getDigital(3) << 3) + (getDigital(4) << 4) + (getDigital(5) << 5);
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 2; //digital inputs
            transmitBuffer[transmitBufferLength++] = temp8;
            temp8 = checksumCalc(buff, 2);
            transmitBuffer[transmitBufferLength++]  = temp8;
            state = IDLE;
            break;
        case PROTO_ANA_INPUTS:
            //immediately return data on analog inputs
            temp16 = 0;// getAnalog(0);  // Analogue input 1
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 3;
            transmitBuffer[transmitBufferLength++] = temp16 & 0xFF;
            transmitBuffer[transmitBufferLength++] = uint8_t(temp16 >> 8);
            temp16 = 0;//getAnalog(1);  // Analogue input 2
            transmitBuffer[transmitBufferLength++] = temp16 & 0xFF;
            transmitBuffer[transmitBufferLength++] = uint8_t(temp16 >> 8);
            temp16 = 0;//getAnalog(2);  // Analogue input 3
            transmitBuffer[transmitBufferLength++] = temp16 & 0xFF;
            transmitBuffer[transmitBufferLength++] = uint8_t(temp16 >> 8);
            temp16 = 0;//getAnalog(3);  // Analogue input 4
            transmitBuffer[transmitBufferLength++] = temp16 & 0xFF;
            transmitBuffer[transmitBufferLength++] = uint8_t(temp16 >> 8);
            temp16 = 0;//getAnalog(4);  // Analogue input 5
            transmitBuffer[transmitBufferLength++] = temp16 & 0xFF;
            transmitBuffer[transmitBufferLength++] = uint8_t(temp16 >> 8);
            temp16 = 0;//getAnalog(5);  // Analogue input 6
            transmitBuffer[transmitBufferLength++] = temp16 & 0xFF;
            transmitBuffer[transmitBufferLength++] = uint8_t(temp16 >> 8);
            temp16 = 0;//getAnalog(6);  // Vehicle Volts
            transmitBuffer[transmitBufferLength++] = temp16 & 0xFF;
            transmitBuffer[transmitBufferLength++] = uint8_t(temp16 >> 8);
            temp8 = checksumCalc(buff, 9);
            transmitBuffer[transmitBufferLength++] = temp8;
            state = IDLE;
            break;
        case PROTO_SET_DIG_OUT:
            state = SET_DIG_OUTPUTS;
            buff[0] = 0xF1;
            break;
        case PROTO_SETUP_CANBUS:
            state = SETUP_CANBUS;
            step = 0;
            buff[0] = 0xF1;
            break;
        case PROTO_GET_CANBUS_PARAMS:
            //immediately return data on canbus params
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 6;
            transmitBuffer[transmitBufferLength++] = settings.CAN0_Enabled + ((unsigned char) settings.CAN0ListenOnly << 4);
            transmitBuffer[transmitBufferLength++] = settings.CAN0Speed;
            transmitBuffer[transmitBufferLength++] = settings.CAN0Speed >> 8;
            transmitBuffer[transmitBufferLength++] = settings.CAN0Speed >> 16;
            transmitBuffer[transmitBufferLength++] = settings.CAN0Speed >> 24;
            transmitBuffer[transmitBufferLength++] = 0;
            transmitBuffer[transmitBufferLength++] = settings.CAN1Speed;
            transmitBuffer[transmitBufferLength++] = settings.CAN1Speed >> 8;
            transmitBuffer[transmitBufferLength++] = settings.CAN1Speed >> 16;
            transmitBuffer[transmitBufferLength++] = settings.CAN1Speed >> 24;
            state = IDLE;
            break;
        case PROTO_GET_DEV_INFO:
            //immediately return device information
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 7;
            transmitBuffer[transmitBufferLength++] = CFG_BUILD_NUM & 0xFF;
            transmitBuffer[transmitBufferLength++] = (CFG_BUILD_NUM >> 8);
            transmitBuffer[transmitBufferLength++] = 0x20;
            transmitBuffer[transmitBufferLength++] = 0;
            transmitBuffer[transmitBufferLength++] = 0;
            transmitBuffer[transmitBufferLength++] = 0; //was single wire mode. Should be rethought for this board.
            state = IDLE;
            break;
        case PROTO_SET_SW_MODE:
            buff[0] = 0xF1;
            state = SET_SINGLEWIRE_MODE;
            step = 0;
            break;
        case PROTO_KEEPALIVE:
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 0x09;
            transmitBuffer[transmitBufferLength++] = 0xDE;
            transmitBuffer[transmitBufferLength++] = 0xAD;
            state = IDLE;
            break;
        case PROTO_SET_SYSTYPE:
            buff[0] = 0xF1;
            state = SET_SYSTYPE;
            step = 0;
            break;
        case PROTO_ECHO_CAN_FRAME:
            state = ECHO_CAN_FRAME;
            buff[0] = 0xF1;
            step = 0;
            break;
        case PROTO_GET_NUMBUSES:
            transmitBuffer[transmitBufferLength++] = 0xF1;
            transmitBuffer[transmitBufferLength++] = 12;
            transmitBuffer[transmitBufferLength++] = SysSettings.numBuses;
            state = IDLE;
            break;
        case PROTO_GET_EXT_BUSES:
            transmitBuffer[transmitBufferLength++]  = 0xF1;
            transmitBuffer[transmitBufferLength++]  = 13;
            for (int u = 2; u < 17; u++) transmitBuffer[transmitBufferLength++] = 0;
            step = 0;
            state = IDLE;
            break;
        case PROTO_SET_EXT_BUSES:
            state = SETUP_EXT_BUSES;
            step = 0;
            buff[0] = 0xF1;
            break;
        }
        break;
    case BUILD_CAN_FRAME:
        buff[1 + step] = in_byte;
        switch(step)
        {
        case 0:
            build_out_frame.id = in_byte;
            break;
        case 1:
            build_out_frame.id |= in_byte << 8;
            break;
        case 2:
            build_out_frame.id |= in_byte << 16;
            break;
        case 3:
            build_out_frame.id |= in_byte << 24;
            if(build_out_frame.id & 1 << 31)
            {
                build_out_frame.id &= 0x7FFFFFFF;
                build_out_frame.extended = true;
            } else build_out_frame.extended = false;
            break;
        case 4:
            out_bus = in_byte & 3;
            break;
        case 5:
            build_out_frame.length = in_byte & 0xF;
            if(build_out_frame.length > 8) 
            {
                build_out_frame.length = 8;
            }
            break;
        default:
            if(step < build_out_frame.length + 6)
            {
                build_out_frame.data.uint8[step - 6] = in_byte;
            } 
            else
            {
                state = IDLE;
                //this would be the checksum byte. Compute and compare.
                //temp8 = checksumCalc(buff, step);
                build_out_frame.rtr = 0;
                if (out_bus == 0) canManager.sendFrame(&CAN0, build_out_frame);
                if (out_bus == 1) canManager.sendFrame(&CAN1, build_out_frame);
            }
            break;
        }
        step++;
        break;
        case TIME_SYNC:
            state = IDLE;
            break;
        case GET_DIG_INPUTS:
            // nothing to do
            break;
        case GET_ANALOG_INPUTS:
            // nothing to do
            break;
        case SET_DIG_OUTPUTS: //todo: validate the XOR byte
            buff[1] = in_byte;
            //temp8 = checksumCalc(buff, 2);
            for(int c = 0; c < 8; c++){
                if(in_byte & (1 << c)) setOutput(c, true);
                else setOutput(c, false);
            }
            state = IDLE;
            break;
        case SETUP_CANBUS: //todo: validate checksum
            switch(step)
            {
            case 0:
                build_int = in_byte;
                break;
            case 1:
                build_int |= in_byte << 8;
                break;
            case 2:
                build_int |= in_byte << 16;
                break;
            case 3:
                build_int |= in_byte << 24;
                busSpeed = build_int & 0xFFFFF;
                if(busSpeed > 1000000) busSpeed = 1000000;

                if(build_int > 0)
                {
                    if(build_int & 0x80000000ul) //signals that enabled and listen only status are also being passed
                    {
                        if(build_int & 0x40000000ul)
                        {
                            settings.CAN0_Enabled = true;
                        } else 
                        {
                            settings.CAN0_Enabled = false;
                        }
                        if(build_int & 0x20000000ul)
                        {
                            settings.CAN0ListenOnly = true;
                        } else 
                        {
                            settings.CAN0ListenOnly = false;
                        }
                    } else 
                    {
                        //if not using extended status mode then just default to enabling - this was old behavior
                        settings.CAN0_Enabled = true;
                    }
                    //CAN0.set_baudrate(build_int);
                    settings.CAN0Speed = busSpeed;
                } else { //disable first canbus
                    settings.CAN0_Enabled = false;
                }

                if (settings.CAN0_Enabled)
                {
                    CAN0.begin(settings.CAN0Speed, 255);
                    if (settings.CAN0ListenOnly) CAN0.setListenOnlyMode(true);
                    else CAN0.setListenOnlyMode(false);
                    CAN0.watchFor();
                }
                else CAN0.disable();
                break;
            case 4:
                build_int = in_byte;
                break;
            case 5:
                build_int |= in_byte << 8;
                break;
            case 6:
                build_int |= in_byte << 16;
                break;
            case 7:
                build_int |= in_byte << 24;
                busSpeed = build_int & 0xFFFFF;
                if(busSpeed > 1000000) busSpeed = 1000000;

                if(build_int > 0 && SysSettings.numBuses > 1)
                {
                    if(build_int & 0x80000000ul) //signals that enabled and listen only status are also being passed
                    {
                        if(build_int & 0x40000000ul)
                        {
                            settings.CAN1_Enabled = true;
                        } else 
                        {
                            settings.CAN1_Enabled = false;
                        }
                        if(build_int & 0x20000000ul)
                        {
                            settings.CAN1ListenOnly = true;
                        } else 
                        {
                            settings.CAN1ListenOnly = false;
                        }
                    } else 
                    {
                        //if not using extended status mode then just default to enabling - this was old behavior
                        settings.CAN1_Enabled = true;
                    }
                    //CAN1.set_baudrate(build_int);
                    settings.CAN1Speed = busSpeed;
                } else { //disable first canbus
                    settings.CAN1_Enabled = false;
                }

                if (settings.CAN1_Enabled)
                {
                    CAN1.begin(settings.CAN0Speed, 255);
                    if (settings.CAN1ListenOnly) CAN1.setListenOnlyMode(true);
                    else CAN1.setListenOnlyMode(false);
                    CAN1.watchFor();
                }
                else CAN1.disable();

                state = IDLE;
                //now, write out the new canbus settings to EEPROM
                //EEPROM.writeBytes(0, &settings, sizeof(settings));
                //EEPROM.commit();
                //setPromiscuousMode();
                break;
            }
            step++;
            break;
        case GET_CANBUS_PARAMS:
            // nothing to do
            break;
        case GET_DEVICE_INFO:
            // nothing to do
            break;
        case SET_SINGLEWIRE_MODE:
            if(in_byte == 0x10){
            } else {
            }
            //EEPROM.writeBytes(0, &settings, sizeof(settings));
            //EEPROM.commit();
            state = IDLE;
            break;
        case SET_SYSTYPE:
            settings.systemType = in_byte;
            //EEPROM.writeBytes(0, &settings, sizeof(settings));
            //EEPROM.commit();
            //loadSettings();
            state = IDLE;
            break;
        case ECHO_CAN_FRAME:
            buff[1 + step] = in_byte;
            switch(step)
            {
            case 0:
                build_out_frame.id = in_byte;
                break;
            case 1:
                build_out_frame.id |= in_byte << 8;
                break;
            case 2:
                build_out_frame.id |= in_byte << 16;
                break;
            case 3:
                build_out_frame.id |= in_byte << 24;
                if(build_out_frame.id & 1 << 31) {
                    build_out_frame.id &= 0x7FFFFFFF;
                    build_out_frame.extended = true;
                } else build_out_frame.extended = false;
                break;
            case 4:
                out_bus = in_byte & 1;
                break;
            case 5:
                build_out_frame.length = in_byte & 0xF;
                if(build_out_frame.length > 8) build_out_frame.length = 8;
                break;
            default:
                if(step < build_out_frame.length + 6) 
                {
                    build_out_frame.data.bytes[step - 6] = in_byte;
                } 
                else 
                {
                    state = IDLE;
                    //this would be the checksum byte. Compute and compare.
                    //temp8 = checksumCalc(buff, step);
                    //if (temp8 == in_byte)
                    //{
                    toggleRXLED();
                    //if(isConnected) {
                    canManager.displayFrame(build_out_frame, 0);
                    //}
                    //}
                }
                break;
            }
            step++;
            break;
        case SETUP_EXT_BUSES: //setup enable/listenonly/speed for SWCAN, Enable/Speed for LIN1, LIN2
            switch(step)
            {
            case 0:
                build_int = in_byte;
                break;
            case 1:
                build_int |= in_byte << 8;
                break;
            case 2:
                build_int |= in_byte << 16;
                break;
            case 3:
                build_int |= in_byte << 24;
                break;
            case 4:
                build_int = in_byte;
                break;
            case 5:
                build_int |= in_byte << 8;
                break;
            case 6:
                build_int |= in_byte << 16;
                break;
            case 7:
                build_int |= in_byte << 24;
                break;
            case 8:
                build_int = in_byte;
                break;
            case 9:
                build_int |= in_byte << 8;
                break;
            case 10:
                build_int |= in_byte << 16;
                break;
            case 11:
                build_int |= in_byte << 24;
                state = IDLE;
                //now, write out the new canbus settings to EEPROM
                //EEPROM.writeBytes(0, &settings, sizeof(settings));
                //EEPROM.commit();
                //setPromiscuousMode();
                break;
            }
        step++;
        break;
    }
}

void GVRET_Comm_Handler::sendFrameToBuffer(CAN_FRAME &frame, int whichBus)
{
    uint8_t temp;
    size_t writtenBytes;
    if (settings.useBinarySerialComm) {
        if (frame.extended) frame.id |= 1 << 31;
        transmitBuffer[transmitBufferLength++] = 0xF1;
        transmitBuffer[transmitBufferLength++] = 0; //0 = canbus frame sending
        uint32_t now = micros();
        transmitBuffer[transmitBufferLength++] = (uint8_t)(now & 0xFF);
        transmitBuffer[transmitBufferLength++] = (uint8_t)(now >> 8);
        transmitBuffer[transmitBufferLength++] = (uint8_t)(now >> 16);
        transmitBuffer[transmitBufferLength++] = (uint8_t)(now >> 24);
        transmitBuffer[transmitBufferLength++] = (uint8_t)(frame.id & 0xFF);
        transmitBuffer[transmitBufferLength++] = (uint8_t)(frame.id >> 8);
        transmitBuffer[transmitBufferLength++] = (uint8_t)(frame.id >> 16);
        transmitBuffer[transmitBufferLength++] = (uint8_t)(frame.id >> 24);
        transmitBuffer[transmitBufferLength++] = frame.length + (uint8_t)(whichBus << 4);
        for (int c = 0; c < frame.length; c++) {
            transmitBuffer[transmitBufferLength++] = frame.data.uint8[c];
        }
        //temp = checksumCalc(buff, 11 + frame.length);
        temp = 0;
        transmitBuffer[transmitBufferLength++] = temp;
        //Serial.write(buff, 12 + frame.length);
    } else {
        writtenBytes = sprintf((char *)&transmitBuffer[transmitBufferLength], "%d - %x", micros(), frame.id);
        transmitBufferLength += writtenBytes;
        if (frame.extended) sprintf((char *)&transmitBuffer[transmitBufferLength], " X ");
        else sprintf((char *)&transmitBuffer[transmitBufferLength], " S ");
        transmitBufferLength += 3;
        writtenBytes = sprintf((char *)&transmitBuffer[transmitBufferLength], "%i %i", whichBus, frame.length);
        transmitBufferLength += writtenBytes;
        for (int c = 0; c < frame.length; c++) {
            writtenBytes = sprintf((char *)&transmitBuffer[transmitBufferLength], " %x", frame.data.uint8[c]);
            transmitBufferLength += writtenBytes;
        }
        sprintf((char *)&transmitBuffer[transmitBufferLength], "\r\n");
        transmitBufferLength += 2;
    }
}

size_t GVRET_Comm_Handler::numAvailableBytes()
{
    return transmitBufferLength;
}

void GVRET_Comm_Handler::clearBufferedBytes()
{
    transmitBufferLength = 0;
}

uint8_t* GVRET_Comm_Handler::getBufferedBytes()
{
    return transmitBuffer;
}

//Get the value of XOR'ing all the bytes together. This creates a reasonable checksum that can be used
//to make sure nothing too stupid has happened on the comm.
uint8_t GVRET_Comm_Handler::checksumCalc(uint8_t *buffer, int length)
{
    uint8_t valu = 0;
    for (int c = 0; c < length; c++) {
        valu ^= buffer[c];
    }
    return valu;
}