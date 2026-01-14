#pragma once
#include <Arduino.h>
#include "config.h"
#include "esp32_can.h"
#include "commbuffer.h"

enum STATE {
    IDLE,
    GET_COMMAND,
    BUILD_CAN_FRAME,
    TIME_SYNC,
    GET_DIG_INPUTS,
    GET_ANALOG_INPUTS,
    SET_DIG_OUTPUTS,
    SETUP_CANBUS,
    GET_CANBUS_PARAMS,
    GET_DEVICE_INFO,
    SET_SINGLEWIRE_MODE,
    SET_SYSTYPE,
    ECHO_CAN_FRAME,
    SETUP_EXT_BUSES
};

enum GVRET_PROTOCOL
{
    PROTO_BUILD_CAN_FRAME = 0,
    PROTO_TIME_SYNC = 1,
    PROTO_DIG_INPUTS = 2,
    PROTO_ANA_INPUTS = 3,
    PROTO_SET_DIG_OUT = 4,
    PROTO_SETUP_CANBUS = 5,
    PROTO_GET_CANBUS_PARAMS = 6,
    PROTO_GET_DEV_INFO = 7,
    PROTO_SET_SW_MODE = 8,
    PROTO_KEEPALIVE = 9,
    PROTO_SET_SYSTYPE = 10,
    PROTO_ECHO_CAN_FRAME = 11,
    PROTO_GET_NUMBUSES = 12,
    PROTO_GET_EXT_BUSES = 13,
    PROTO_SET_EXT_BUSES = 14,
    PROTO_BUILD_FD_FRAME = 20,
    PROTO_SETUP_FD = 21,
    PROTO_GET_FD = 22,
};

class GVRET_Comm_Handler: public CommBuffer
{
public:
    GVRET_Comm_Handler();
    void processIncomingByte(uint8_t in_byte);
    
private:
    CAN_FRAME build_out_frame;
    CAN_FRAME_FD build_out_fd_frame;
    int out_bus;
    uint8_t buff[20];
    int step;
    STATE state;
    uint32_t build_int;

    uint8_t checksumCalc(uint8_t *buffer, int length);
};