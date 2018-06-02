/*
 * ApplySignal.ino
 * 2018 WLWilliams
 * 
 * This sketch demonstrates the basic use of the AD9833 DDS module library.
 * Using the ApplySignal to generate and/or change the signal.
 * 
 * This program is free software: you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version. 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * This example code is in the public domain.
 * 
 * Library code found at: https://github.com/Billwilliams1952/AD9833-Library-Arduino
 * 
 */

#include <AD9833.h>     // Include the library

#define FNC_PIN 4       // Can be any digital IO pin

//--------------- Create an AD9833 object ---------------- 
// Note, SCK and MOSI must be connected to CLK and DAT pins on the AD9833 for SPI
AD9833 gen(FNC_PIN);       // Defaults to 25MHz internal reference frequency

void setup() {
    // This MUST be the first command after declaring the AD9833 object
    gen.Begin();              

    // Apply a 1000 Hz sine wave using REG0 (register set 0). There are two register sets,
    // REG0 and REG1. 
    // Each one can be programmed for:
    //   Signal type - SINE_WAVE, TRIANGLE_WAVE, SQUARE_WAVE, and HALF_SQUARE_WAVE
    //   Frequency - 0 to 12.5 MHz
    //   Phase - 0 to 360 degress (this is only useful if it is 'relative' to some other signal
    //           such as the phase difference between REG0 and REG1).
    // In ApplySignal, if Phase is not given, it defaults to 0.
    gen.ApplySignal(SINE_WAVE,REG0,1000);
   
    gen.EnableOutput(true);   // Turn ON the output - it defaults to OFF
    // There should be a 1000 Hz sine wave on the output of the AD9833
}

void loop() {
    // To change the signal, you can just call ApplySignal again with a new frequency and/or signal
    // type.
}
