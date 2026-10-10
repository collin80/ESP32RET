#pragma once
#include "config.h"

typedef struct {
    uint32_t bitsPerQuarter;
    uint32_t bitsSoFar;
    uint8_t busloadPercentage;
} BUSLOAD;

class CAN_COMMON;
class CAN_FRAME;
class CAN_FRAME_FD;

class CANManager
{
public:
    CANManager();
    void addBits(int offset, CAN_FRAME &frame);
    void addBits(int offset, CAN_FRAME_FD &frame);    
    void sendFrame(CAN_COMMON *bus, CAN_FRAME &frame);
    void sendFrame(CAN_COMMON *bus, CAN_FRAME_FD &frame);
    void displayFrame(CAN_FRAME &frame, int whichBus);
    void displayFrame(CAN_FRAME_FD &frame, int whichBus);
    void loop();
    void setup();
    void restartBus(int i);
    void setSendToConsole(bool state) { sendToConsole = state; }
    void printStats(bool reset);

private:
    uint32_t rxCount[NUM_BUSES] = {0};  //frames read from each bus since boot / last STATS=1
    uint32_t txCount[NUM_BUSES] = {0};  //frames handed to each bus for sending
    BUSLOAD busLoad[NUM_BUSES];
    uint32_t busLoadTimer;
    bool sendToConsole;
};
