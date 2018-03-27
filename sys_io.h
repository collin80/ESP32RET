/*
 * sys_io.h
 *
 * Handles raw interaction with system I/O
 *
Copyright (c) 2013 Collin Kidder, Michael Neuweiler, Charles Galpin

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


#ifndef SYS_IO_H_
#define SYS_IO_H_

#include <Arduino.h>
#include "config.h"
#include "Logger.h"

typedef struct {
    uint16_t offset;
    uint16_t gain;
} ADC_COMP;

extern void ADC_Handler();
void sys_early_setup();
void setup_sys_io();
void setupFastADC();
void getADCAvg();  //CALL in loop()Take the arithmetic average of the readings in the buffer for each channel & updates adc_out_vals[x]
uint16_t getAnalog(uint8_t which); //get value of one of the 9 analog inputs
boolean getDigital(uint8_t which);  ////get value of one of the 6 digital/sudo(Analogue) inputs 0->(NUM_DIGITAL - 1)
void setOutput(uint8_t which, boolean active); //set output high or not
boolean getOutput(uint8_t which); //get current value of output state (high?)
void setLED(uint8_t, boolean);

#endif

