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
    if (settings.CAN0_Enabled)
    {
        if (settings.systemType == 0) CAN0.setCANPins(GPIO_NUM_4, GPIO_NUM_5);
        CAN0.enable();
        CAN0.begin(settings.CAN0Speed, 255);
        Serial.print("Enabled CAN0 with speed ");
        Serial.println(settings.CAN0Speed);
        if (settings.CAN0ListenOnly) 
        {
            CAN0.setListenOnlyMode(true);
        } 
        else 
        {
            CAN0.setListenOnlyMode(false);
        }
        CAN0.watchFor();
    } 
    else
    {
        CAN0.disable();
    }

    if (settings.CAN1_Enabled && settings.systemType != 0) {
        CAN1.enable();
        CAN1.begin(settings.CAN1Speed, 255);
        Serial.print("Enabled CAN1 with speed ");
        Serial.println(settings.CAN1Speed);
        if (settings.CAN1ListenOnly)
        {
            CAN1.setListenOnlyMode(true);
        }
        else
        {
            CAN1.setListenOnlyMode(false);
        }
        CAN1.watchFor();
    } 
    else
    {
        CAN1.disable();
    }

    busLoad[0].bitsPerQuarter = settings.CAN0Speed / 4;
    busLoad[1].bitsPerQuarter = settings.CAN1Speed / 4;

    for (int j = 0; j < NUM_BUSES; j++)
    {
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

void CANManager::sendFrame(CAN_COMMON *bus, CAN_FRAME &frame)
{
    int whichBus;
    if (bus == &CAN0) whichBus = 0;
    if (bus == &CAN1) whichBus = 1;
    bus->sendFrame(frame);
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

void CANManager::loop()
{
    CAN_FRAME incoming;
    size_t wifiLength = wifiGVRET.numAvailableBytes();
    size_t serialLength = serialGVRET.numAvailableBytes();
    size_t maxLength = (wifiLength>serialLength)?wifiLength:serialLength;

    if (millis() > (busLoadTimer + 250)) {
        busLoadTimer = millis();
        busLoad[0].busloadPercentage = ((busLoad[0].busloadPercentage * 3) + (((busLoad[0].bitsSoFar * 1000) / busLoad[0].bitsPerQuarter) / 10)) / 4;
        //Force busload percentage to be at least 1% if any traffic exists at all. This forces the LED to light up for any traffic.
        if (busLoad[0].busloadPercentage == 0 && busLoad[0].bitsSoFar > 0) busLoad[0].busloadPercentage = 1;
        busLoad[0].bitsPerQuarter = settings.CAN0Speed / 4;
        busLoad[0].bitsSoFar = 0;
        if(busLoad[0].busloadPercentage > busLoad[1].busloadPercentage){
            //updateBusloadLED(busLoad[0].busloadPercentage);
        } else{
            //updateBusloadLED(busLoad[1].busloadPercentage);
        }
    }

    while (CAN0.available() > 0 && (maxLength < (WIFI_BUFF_SIZE - 80)))
    {
        CAN0.read(incoming);
        addBits(0, incoming);
        toggleRXLED();
        displayFrame(incoming, 0);
        if (incoming.id > 0x7DF && incoming.id < 0x7F0) elmEmulator.processCANReply(incoming);
        wifiLength = wifiGVRET.numAvailableBytes();
        serialLength = serialGVRET.numAvailableBytes();
        maxLength = (wifiLength > serialLength) ? wifiLength:serialLength;
    }

    if (settings.systemType != 0)
    {
        while (CAN1.available() > 0 && (maxLength < (WIFI_BUFF_SIZE - 80)))
        {
            CAN1.read(incoming);
            addBits(1, incoming);
            toggleRXLED();
            displayFrame(incoming, 1);
            if (incoming.id > 0x7DF && incoming.id < 0x7F0) elmEmulator.processCANReply(incoming);
            wifiLength = wifiGVRET.numAvailableBytes();
            serialLength = serialGVRET.numAvailableBytes();
            maxLength = (wifiLength > serialLength) ? wifiLength:serialLength;
        }
    }
}
