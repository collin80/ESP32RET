#include <Arduino.h>
#include "can_manager.h"
#include "esp32_can.h"
#include "config.h"
#include "SerialConsole.h"
#include "gvret_comm.h"
#include "lawicel.h"
#include "ELM327_Emulator.h"


//twai alerts copied here for ease of access. Look up alerts right here:
//#define TWAI_ALERT_TX_IDLE                  0x00000001  /**< Alert(1): No more messages to transmit */
//#define TWAI_ALERT_TX_SUCCESS               0x00000002  /**< Alert(2): The previous transmission was successful */
//#define TWAI_ALERT_RX_DATA                  0x00000004  /**< Alert(4): A frame has been received and added to the RX queue */
//#define TWAI_ALERT_BELOW_ERR_WARN           0x00000008  /**< Alert(8): Both error counters have dropped below error warning limit */
//#define TWAI_ALERT_ERR_ACTIVE               0x00000010  /**< Alert(16): TWAI controller has become error active */
//#define TWAI_ALERT_RECOVERY_IN_PROGRESS     0x00000020  /**< Alert(32): TWAI controller is undergoing bus recovery */
//#define TWAI_ALERT_BUS_RECOVERED            0x00000040  /**< Alert(64): TWAI controller has successfully completed bus recovery */
//#define TWAI_ALERT_ARB_LOST                 0x00000080  /**< Alert(128): The previous transmission lost arbitration */
//#define TWAI_ALERT_ABOVE_ERR_WARN           0x00000100  /**< Alert(256): One of the error counters have exceeded the error warning limit */
//#define TWAI_ALERT_BUS_ERROR                0x00000200  /**< Alert(512): A (Bit, Stuff, CRC, Form, ACK) error has occurred on the bus */
//#define TWAI_ALERT_TX_FAILED                0x00000400  /**< Alert(1024): The previous transmission has failed (for single shot transmission) */
//#define TWAI_ALERT_RX_QUEUE_FULL            0x00000800  /**< Alert(2048): The RX queue is full causing a frame to be lost */
//#define TWAI_ALERT_ERR_PASS                 0x00001000  /**< Alert(4096): TWAI controller has become error passive */
//#define TWAI_ALERT_BUS_OFF                  0x00002000  /**< Alert(8192): Bus-off condition occurred. TWAI controller can no longer influence bus */
//#define TWAI_ALERT_RX_FIFO_OVERRUN          0x00004000  /**< Alert(16384): An RX FIFO overrun has occurred */
//#define TWAI_ALERT_TX_RETRIED               0x00008000  /**< Alert(32768): An message transmission was cancelled and retried due to an errata workaround */
//#define TWAI_ALERT_PERIPH_RESET             0x00010000  /**< Alert(65536): The TWAI controller was reset */
//#define TWAI_ALERT_ALL                      0x0001FFFF  /**< Bit mask to enable all alerts during configuration */
//#define TWAI_ALERT_NONE                     0x00000000  /**< Bit mask to disable all alerts during configuration */
//#define TWAI_ALERT_AND_LOG                  0x00020000  /**< Bit mask to enable alerts to also be logged when they occur. Note that logging from the ISR is disabled if CONFIG_TWAI_ISR_IN_IRAM is enabled (see docs). */


CANManager::CANManager()
{

}

void CANManager::setup()
{
    for (int i = 0; i < SysSettings.numBuses; i++)
    {
        if (settings.canSettings[i].enabled)
        {
            if ((settings.canSettings[i].fdMode == 0) || !canBuses[i]->supportsFDMode())
            {
                canBuses[i]->begin(settings.canSettings[i].nomSpeed);
                Serial.printf("Enabled CAN%u with speed %u\n", i, settings.canSettings[i].nomSpeed);
                if ( (i == 0) && (settings.systemType == 2) )
                {
                  digitalWrite(SW_EN, HIGH); //MUST be HIGH to use CAN0 channel
                  Serial.println("Enabling SWCAN Mode");
                }
                if ( (i == 1) && (settings.systemType == 2) )
                {
                  digitalWrite(SW_EN, LOW); //MUST be LOW to use CAN1 channel
                  Serial.println("Enabling CAN1 will force CAN0 off.");
                }
                canBuses[i]->enable();
            }
            else
            {
                canBuses[i]->beginFD(settings.canSettings[i].nomSpeed, settings.canSettings[i].fdSpeed);
                Serial.printf("Enabled CAN1 In FD Mode With Nominal Speed %u and Data Speed %u", 
                                settings.canSettings[i].nomSpeed, settings.canSettings[i].fdSpeed);
                canBuses[i]->enable();
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
        uint8_t stdbymode;
        //need to set all MCP2517FD modules to use GPIO0 as XSTBY to control transceivers
        for (int i = 1; i < 5; i++)
        {
            MCP2517FD *can = (MCP2517FD *)canBuses[i];
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

    for (int i = 0; i < SysSettings.numBuses; i++)
    {
        if (!canBuses[i]) continue;
        if (!settings.canSettings[i].enabled) continue;
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
