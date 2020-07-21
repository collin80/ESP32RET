#pragma once
#include "config.h"

typedef struct {
    uint32_t bitsPerQuarter;
    uint32_t bitsSoFar;
    uint8_t busloadPercentage;
} BUSLOAD;

class CAN_COMMON;
class CAN_FRAME;

class CANManager
{
public:
    CANManager();
    void addBits(int offset, CAN_FRAME &frame);
    void sendFrame(CAN_COMMON *bus, CAN_FRAME &frame);
    void displayFrame(CAN_FRAME &frame, int whichBus);
    void loop();
    void setup();

private:
    BUSLOAD busLoad[NUM_BUSES];
    uint32_t busLoadTimer;
};
