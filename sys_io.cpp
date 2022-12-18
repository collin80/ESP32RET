/*
 * sys_io.cpp
 *
 * Handles the low level details of system I/O
 *
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

some portions based on code credited as:
Arduino Due ADC->DMA->USB 1MSPS
by stimmer

*/

#include "sys_io.h"
#include <FastLED.h>

extern CRGB leds[A5_NUM_LEDS];

bool useRawADC = false;

#undef HID_ENABLED

uint8_t dig[NUM_DIGITAL];   //digital inputs. sudo/analogue inputs
uint8_t adc[NUM_ANALOG][2]; // [x][0]This holds the ADC Channel number. // This is calculated in sys_early_setup()
                            // [x][1]This holds the pointer of the ADC number position into the adc_buf[x][x] array (Now redundent).
                            // This is calculated in sys_early_setup()
uint8_t out[NUM_OUTPUT];    //digital output configuration details

uint32_t Enabled_Analogue_Pins = 0; // Sum of ADC input numbers to load into ADC_CHER

ADC_COMP adc_comp[NUM_ANALOG]; //holds ADC offset or gain

//forces the digital I/O ports to a safe state. This is called very early in initialization.
void sys_early_setup(){
    uint8_t i;
}

/*
Initialize DMA driven ADC and read in gain/offset for each channel
*/
void setup_sys_io()
{
    uint8_t i;

    setupFastADC();
}

/*
Setup the system to continuously read the proper ADC channels and use DMA to place the results into RAM
Testing to find a good batch of settings for how fast to do ADC readings. The relevant areas:
1. In the adc_init call it is possible to use something other than ADC_FREQ_MAX to slow down the ADC clock
2. ADC_MR has a clock divisor, start up time, settling time, tracking time, and transfer time. These can be adjusted
*/
void setupFastADC(){
    Logger::debug("Fast ADC Mode Enabled");
}

/*
get value of one of the 9 analog inputs 0->(NUM_ANALOG - 1)
Uses a special buffer which has smoothed and corrected ADC values. This call is very fast
because the actual work is done via DMA and then a separate polled step.
*/
uint16_t getAnalog(uint8_t which)
{

    if (which >= NUM_ANALOG) which = 0;
    if (adc[which][0] > 15) which = 0;

    return 0;//adc_out_vals[which];
}

/*
get value of one of the sudo 6 digital/Analogue inputs 0->(NUM_DIGITAL - 1)
*/
boolean getDigital(uint8_t which)
{
    if((which >= NUM_OUTPUT) || (dig[which] == 255)){
        return(false);
    }
    return(false); //(adc_out_vals[which] > 200) ? true : false);
}

//set output high or not
void setOutput(uint8_t which, boolean active)
{
    if((which >= NUM_OUTPUT) || (out[which] == 255)){
        return;
    }
    if(active){
        (which <= 2) ? digitalWrite(out[which], HIGH) : digitalWrite(out[which], LOW);
    }else{
        (which <= 2) ? digitalWrite(out[which], LOW) : digitalWrite(out[which], HIGH);
    }
}

//get current value of output state (high?)
boolean getOutput(uint8_t which)
{
    if((which >= NUM_OUTPUT) || (out[which] == 255)){
        return false;
    }
    return digitalRead(out[which]);
}

void setLED(uint8_t which, boolean hi){
    if(which == 255){
        return;
    }
    if(hi){
        digitalWrite(which, HIGH);
    } else{
        digitalWrite(which, LOW);
    }
}

void toggleRXLED()
{
    static int counter = 0;
    counter++;
    if (counter >= BLINK_SLOWNESS) {
        counter = 0;
        SysSettings.rxToggle = !SysSettings.rxToggle;
        if (!SysSettings.fancyLED) setLED(SysSettings.LED_CANRX, SysSettings.rxToggle);
        else
        {
          leds[SysSettings.LED_CANRX] = SysSettings.rxToggle?CRGB::Blue:CRGB::Black;
        };
    }
}

void toggleTXLED()
{
    static int counter = 0;
    counter++;
    if (counter >= BLINK_SLOWNESS) {
        counter = 0;
        SysSettings.txToggle = !SysSettings.txToggle;
        if (!SysSettings.fancyLED) setLED(SysSettings.LED_CANTX, SysSettings.txToggle);
        else
        {
          leds[SysSettings.LED_CANRX] = SysSettings.rxToggle?CRGB::Green:CRGB::Black;
        };
    }
}
