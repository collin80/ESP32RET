#include <Arduino.h>
#include "can_manager.h"
#include "esp32_can.h"
#include "config.h"
#include "SerialConsole.h"
#include "gvret_comm.h"
#include "lawicel.h"
#include "ELM327_Emulator.h"

CANManager::CANManager()
{

}

void CANManager::setup()
{
    for (int i = 0; i < SysSettings.numBuses; i++)
    {
        if (settings.canSettings[i].enabled)
        {
            canBuses[i]->enable();
            if (settings.canSettings[i].fdMode == 0)
            {
                canBuses[i]->begin(settings.canSettings[i].nomSpeed, 255);
                Serial.printf("Enabled CAN%u with speed %u\n", i, settings.canSettings[i].nomSpeed);
            }
            else
            {
                canBuses[i]->beginFD(settings.canSettings[i].nomSpeed, settings.canSettings[i].fdSpeed);
                Serial.printf("Enabled CAN1 In FD Mode With Nominal Speed %u and Data Speed %u", 
                                settings.canSettings[i].nomSpeed, settings.canSettings[i].fdSpeed);
            }

            if (settings.canSettings[i].listenOnly) 
            {
                canBuses[i]->setListenOnlyMode(true);
            }
            else
            {
                canBuses[i]->setListenOnlyMode(false);
            }
            canBuses[i]->watchFor();
        } 
        else
        {
            canBuses[i]->disable();
        }
    }

    if (settings.systemType == 2) //Macchina 5-CAN Board
    {
        uint8_t  stdbymode;
        //need to set all MCP2517FD modules to use GPIO0 as XSTBY to control transceivers
        for (int i = 1; i < 5; i++)
        {
            MCP2517FD *can = (MCP2517FD *)canBuses[1];
            stdbymode = can->Read8(0xE04);
            stdbymode |= 0x40; // Set bit 6 to enable XSTBY mode
            can->Write8(0xE04, stdbymode);
            stdbymode = can->Read8(0xE04);
            stdbymode &= 0xFE; // clear low bit so GPIO0 is output
            can->Write8(0xE04, stdbymode);
        }
    }

    for (int j = 0; j < NUM_BUSES; j++)
    {
        busLoad[j].bitsPerQuarter = settings.canSettings[j].nomSpeed / 4;
        busLoad[j].bitsSoFar = 0;
        busLoad[j].busloadPercentage = 0;
        if (busLoad[j].bitsPerQuarter == 0) busLoad[j].bitsPerQuarter = 125000;
    }

    busLoadTimer = millis();
}

void CANManager::addBits(int offset, CAN_FRAME &frame)
{
    if (offset < 0) return;
    if (offset >= NUM_BUSES) return;
    busLoad[offset].bitsSoFar += 41 + (frame.length * 9);
    if (frame.extended) busLoad[offset].bitsSoFar += 18;
}

void CANManager::addBits(int offset, CAN_FRAME_FD &frame)
{
    if (offset < 0) return;
    if (offset >= NUM_BUSES) return;
    busLoad[offset].bitsSoFar += 41 + (frame.length * 9);
    if (frame.extended) busLoad[offset].bitsSoFar += 18;
}

void CANManager::sendFrame(CAN_COMMON *bus, CAN_FRAME &frame)
{
    int whichBus = 0;
    for (int i = 0; i < NUM_BUSES; i++) if (canBuses[i] == bus) whichBus = i;
    bus->sendFrame(frame);
    addBits(whichBus, frame);
}

void CANManager::sendFrame(CAN_COMMON *bus, CAN_FRAME_FD &frame)
{
    int whichBus = 0;
    for (int i = 0; i < NUM_BUSES; i++) if (canBuses[i] == bus) whichBus = i;
    bus->sendFrameFD(frame);
    addBits(whichBus, frame);
}


void CANManager::displayFrame(CAN_FRAME &frame, int whichBus)
{
    if (settings.enableLawicel && SysSettings.lawicelMode) 
    {
        lawicel.sendFrameToBuffer(frame, whichBus);
    } 
    else 
    {
        if (SysSettings.isWifiActive) wifiGVRET.sendFrameToBuffer(frame, whichBus);
        else serialGVRET.sendFrameToBuffer(frame, whichBus);
    }
}

void CANManager::displayFrame(CAN_FRAME_FD &frame, int whichBus)
{
    if (settings.enableLawicel && SysSettings.lawicelMode) 
    {
        //lawicel.sendFrameToBuffer(frame, whichBus);
    } 
    else 
    {
        if (SysSettings.isWifiActive) wifiGVRET.sendFrameToBuffer(frame, whichBus);
        else serialGVRET.sendFrameToBuffer(frame, whichBus);
    }
}

void CANManager::loop()
{
    CAN_FRAME incoming;
    CAN_FRAME_FD inFD;
    size_t wifiLength = wifiGVRET.numAvailableBytes();
    size_t serialLength = serialGVRET.numAvailableBytes();
    size_t maxLength = (wifiLength > serialLength) ? wifiLength : serialLength;

    if (millis() > (busLoadTimer + 250)) {
        busLoadTimer = millis();
        busLoad[0].busloadPercentage = ((busLoad[0].busloadPercentage * 3) + (((busLoad[0].bitsSoFar * 1000) / busLoad[0].bitsPerQuarter) / 10)) / 4;
        //Force busload percentage to be at least 1% if any traffic exists at all. This forces the LED to light up for any traffic.
        if (busLoad[0].busloadPercentage == 0 && busLoad[0].bitsSoFar > 0) busLoad[0].busloadPercentage = 1;
        busLoad[0].bitsPerQuarter = settings.canSettings[0].nomSpeed / 4;
        busLoad[0].bitsSoFar = 0;
        if(busLoad[0].busloadPercentage > busLoad[1].busloadPercentage){
            //updateBusloadLED(busLoad[0].busloadPercentage);
        } else{
            //updateBusloadLED(busLoad[1].busloadPercentage);
        }
    }

    for (int i = 0; i < NUM_BUSES; i++)
    {
        while ( (canBuses[i]->available() > 0) && (maxLength < (WIFI_BUFF_SIZE - 80)))
        {
            if (settings.canSettings[i].fdMode == 0)
            {
                canBuses[i]->read(incoming);
                addBits(i, incoming);
                displayFrame(incoming, i);
            }
            else
            {
                canBuses[i]->readFD(inFD);
                addBits(i, inFD);
                displayFrame(inFD, i);
            }
            toggleRXLED();
            if ( (incoming.id > 0x7DF && incoming.id < 0x7F0) || elmEmulator.getMonitorMode() ) elmEmulator.processCANReply(incoming);
            wifiLength = wifiGVRET.numAvailableBytes();
            serialLength = serialGVRET.numAvailableBytes();
            maxLength = (wifiLength > serialLength) ? wifiLength:serialLength;
        }
    }
}
