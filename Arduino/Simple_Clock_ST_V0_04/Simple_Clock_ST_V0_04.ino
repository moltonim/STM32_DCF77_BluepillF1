//
//  www.blinkenlight.net
//
//  Copyright 2016 Udo Klein
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see http://www.gnu.org/licenses/


/*
  Version Simple-Clock_ST 6v6gt
  This is intended to add support to the Udo Klein dcf77 library https://github.com/udoklein/dcf77  
  for the ST Microsystems Arduino Core (current version 2.2.0)
  https://github.com/stm32duino/Arduino_Core_STM32 for use with the 
  "Bluepill" STM32F103 and Blackpill STM32F411CEU6.
  
  This addition should have no impact on existing version support.
  Note: The ST Microsystems core identifies the STM32F103 "Bluepill" by 
  the macro 'STM32F1' . The earlier "Roger Clark" version of the core used the
  macro '__STM32F1__' . This distinction is retained and, because the ST Microsystems
  core is closer to the standard Arduino, the additions are relatively minor,
  for example, using the normal Serial instead of Serial1.

  Most changes identified with the tag $siST in the code.

  Ver V0_01  25.04.2022  No changes to original code except "dcf77.h" lib call 
  Ver V0_02  25.04.2022  Attempt inclusion of STM32F1 and SFM32F4 in the library
                         https://github.com/stm32duino/wiki/wiki/HardwareTimer-library
                         currently fixed for bluepill @72MHz
                           routines marked $siST_V0_02
  Ver V0_03  27.04.2022  blue/blackpill timer more generic. Test OK with blue and black pill
  Ver V0_04  14.05.2022  Add in dcf77.cpp from Dcf77_BluepillHT1632_V0_04.
                         Tested with Canaduino DCF77 module and both bluepill and uno.
                         Strangely, although both boards are defined as "dcf77_inverted_samples = 1;"
                         the blupill takes the non inverted output of the module and the monitor led
                         has 80%-90% duty cycle. 
                         On the Uno, the inverted output of the module is used and the monitor led
                         exhibits a 10% to 20% duty cycle.
                         Both boards work. Both leds appear to be wired HIGH => led ON.
                         The code appears to show that the monitor led activity should reflect the 
                         resolved state (after inversion/non-inversion) which would be the 80%-90% duty cycle
                         if the LED reflects the amplitude of the original carrier signal.
                         Correction and part explanation: Bluepill led is wired High side so LOW => led ON
                
  ToDo
  TD01: clean and full rejection of unsupported architectures. Unknown arm variants may be treated as Due.
  TD02: critical sections not necessary for protecting up to 32 bit operations on 32bit processors 
  TD03: try to clean up some of the compiler warnings

*/

#include "dcf77.h"   // $siST_V0_01

#if defined(__AVR__)
const uint8_t dcf77_analog_sample_pin = 5;
const uint8_t dcf77_sample_pin = A5;       // A5 == d19
const uint8_t dcf77_inverted_samples = 1;
const uint8_t dcf77_analog_samples = 0;    // $siST_V0_04
// const uint8_t dcf77_pin_mode = INPUT;  // disable internal pull up
const uint8_t dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up

const uint8_t dcf77_monitor_led = 18;  // A4 == d18

uint8_t ledpin(const uint8_t led) {
    return led;
}
#elif defined(__STM32F1__) || defined(STM32F1) || defined(STM32F4)  // $siST_V0_02 block
const uint8_t dcf77_sample_pin = PB6;
const uint8_t dcf77_inverted_samples = 1; //output from HKW EM6 DCF 3V
  #if defined(__STM32F1__)
    const WiringPinMode dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up
  #else // STM32F1/F4
    const uint8_t dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up
  #endif    
const uint8_t dcf77_monitor_led = PC13;
uint8_t ledpin(const int8_t led) {
    return led;
}
#else
const uint8_t dcf77_sample_pin = 53;
const uint8_t dcf77_inverted_samples = 0;

// const uint8_t dcf77_pin_mode = INPUT;  // disable internal pull up
const uint8_t dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up

const uint8_t dcf77_monitor_led = 19;

uint8_t ledpin(const uint8_t led) {
    return led<14? led: led+(54-14);
}
#endif

uint8_t sample_input_pin() {
    const uint8_t sampled_data =
        #if defined(__AVR__)
        dcf77_inverted_samples ^ (dcf77_analog_samples? (analogRead(dcf77_analog_sample_pin) > 200)
                                                      : digitalRead(dcf77_sample_pin));
        #else
        dcf77_inverted_samples ^ digitalRead(dcf77_sample_pin);
        #endif

    digitalWrite(ledpin(dcf77_monitor_led), sampled_data);
    return sampled_data;
}

void setup() {
    using namespace Clock;

    #if defined(__STM32F1__)  
    Serial1.begin(115200);
    #elif  defined(STM32F1) || defined(STM32F4)  // $siST_V0_02 
    Serial.begin(115200);
    while (! Serial && millis() < 5000UL ) ;  // $siST_V0_04
    #else
    Serial.begin(9600);
    #endif
    sprintln();
    sprintln(F("Simple DCF77 Clock V3.1.1"));
    sprintln(F("(c) Udo Klein 2016"));
    sprintln(F("www.blinkenlight.net"));
    sprintln();
    sprint(F("Sample Pin:      ")); sprintln(dcf77_sample_pin);
    sprint(F("Sample Pin Mode: ")); sprintln(dcf77_pin_mode);
    sprint(F("Inverted Mode:   ")); sprintln(dcf77_inverted_samples);
    #if defined(__AVR__)
    sprint(F("Analog Mode:     ")); sprintln(dcf77_analog_samples);
    #endif
    sprint(F("Monitor Pin:     ")); sprintln(ledpin(dcf77_monitor_led));
    sprintln();
    sprintln();
    sprintln(F("Initializing..."));

    pinMode(ledpin(dcf77_monitor_led), OUTPUT);
    pinMode(dcf77_sample_pin, dcf77_pin_mode);

    DCF77_Clock::setup();
    DCF77_Clock::set_input_provider(sample_input_pin);


    // Wait till clock is synced, depending on the signal quality this may take
    // rather long. About 5 minutes with a good signal, 30 minutes or longer
    // with a bad signal
    for (uint8_t state = Clock::useless;
        state == Clock::useless || state == Clock::dirty;
        state = DCF77_Clock::get_clock_state()) {

        // wait for next sec
        Clock::time_t now;
        DCF77_Clock::get_current_time(now);

        // render one dot per second while initializing
        static uint8_t count = 0;
        sprint('.');
        ++count;
        if (count == 60) {
            count = 0;
            sprintln();
        }
    }
}

void paddedPrint(BCD::bcd_t n) {
    sprint(n.digit.hi);
    sprint(n.digit.lo);
}

void loop() {
    Clock::time_t now;

    DCF77_Clock::get_current_time(now);
    if (now.month.val > 0) {
        switch (DCF77_Clock::get_clock_state()) {
            case Clock::useless: sprint(F("useless ")); break;
            case Clock::dirty:   sprint(F("dirty:  ")); break;
            case Clock::synced:  sprint(F("synced: ")); break;
            case Clock::locked:  sprint(F("locked: ")); break;
            case Clock::free:    sprint(F("free:   ")); break;   // $siST_V0_03
            case Clock::unlocked:  sprint(F("unlocked: ")); break;  // $siST_V0_03
            default: sprint(F("unknown: "));  // $siST_V0_03
            
        }
        sprint(' ');

        sprint(F("20"));
        paddedPrint(now.year);
        sprint('-');
        paddedPrint(now.month);
        sprint('-');
        paddedPrint(now.day);
        sprint(' ');

        paddedPrint(now.hour);
        sprint(':');
        paddedPrint(now.minute);
        sprint(':');
        paddedPrint(now.second);

        sprint("+0");
        sprint(now.uses_summertime? '2': '1');
        sprintln();
    }
}
