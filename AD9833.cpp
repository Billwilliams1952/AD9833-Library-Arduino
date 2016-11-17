/*
 * AD9833.cpp
 * 
 * Copyright 2016 Bill Williams <wlwilliams1952@gmail.com, github/BillWilliams1952>
 *
 * TODO: Thanks to *** GET WEBSITE *** for his initial code samples.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */

#include "AD9833.h"

AD9833 :: AD9833 ( uint8_t FNCpin, uint32_t referenceFrequency ) {
	// Pin used to enable SPI communication (active LOW)
	this->FNCpin = FNCpin;
	pinMode(FNCpin,OUTPUT);
	digitalWrite(FNCpin,HIGH);

	/* TODO: The minimum resolution and max frequency are determined by
	 * by referenceFrequency. We should calculate these values and use
	 * them during setFrequency
	 */
	refFrequency = referenceFrequency;
	
	// Setup some defaults
	sleepEnabled = false;
	outputEnabled = false;
	waveForm = SINE_WAVE;
	frequency0 = frequency1 = 1000;		// 1 KHz sine wave to start
	phase0 = phase1 = 0.0;				// 0 phase
	activeFreq = REG0; activePhase = REG0;
}

void AD9833 :: Begin ( void ) {
	SPI.begin();
	delay(100);
	Reset();
}

void AD9833 :: Reset ( void ) {
	WriteRegister(RESET_CMD);   // Write '1' to control reg D8.
	delay(10);					// Is this really needed?
}

/***********************************************************************
						Control Register
------------------------------------------------------------------------
D15,D14(MSB)	10 = FREQ1 write, 01 = FREQ0 write,
 				11 = PHASE write, 00 = control write
D13	If D15,D14 = 00, 0 = individual LSB and MSB FREQ write,
 					 1 = both LSB and MSB FREQ writes consecutively
	If D15,D14 = 11, 0 = PHASE0 write, 1 = PHASE1 write
D12	0 = writing LSB independently
 	1 = writing MSB independently
D11	0 = output FREQ0,
	1 = output FREQ1
D10	0 = output PHASE0
	1 = output PHASE1
D9	Reserved. Must be 0.
D8	0 = RESET disabled
	1 = RESET enabled
D7	0 = internal clock is enabled
	1 = internal clock is disabled
D6	0 = onboard DAC is active for sine and triangle wave output,
 	1 = put DAC to sleep for square wave output
D5	0 = output depends on D1
	1 = output is a square wave
D4	Reserved. Must be 0.
D3	0 = square wave of half frequency output
	1 = square wave output
D2	Reserved. Must be 0.
D1	If D5 = 1, D1 = 0.
	Otherwise 0 = sine output, 1 = triangle output
D0	Reserved. Must be 0.
***********************************************************************/

// Set the frequency and waveform registers in the AD9833.
void AD9833 :: SetFrequency ( Registers freqReg, float frequency ) {
	// TODO: calculate resolution and max frequency based on
	// refFrequency. Use the calculations for sanity checks on numbers.
	if ( frequency > 12.5e6 )		// Sanity check on frequency
		frequency = 12.5e6;
	if ( frequency < 0.1 ) frequency = 0.1;
	if ( freqReg == REG0 ) frequency0 = frequency;
	else frequency1 = frequency;
	
	int32_t freqWord = (frequency * pow2_28) / (float)refFrequency;
	int16_t upper14 = (int16_t)((freqWord & 0xFFFC000) >> 14), 
			lower14 = (int16_t)(freqWord & 0x3FFF);

	// Which register are we updating?
	uint16_t reg = freqReg == REG0 ? FREQ0_WRITE_REG : FREQ1_WRITE_REG;
	lower14 |= reg;
	upper14 |= reg;   

	// The spec says RESET MUST be enabled? I don't enable it and
	// I get smoother transistions in frequency since the output
	// isn't reset.  What am I missing here?
	// Control write: both LSB and MSB FREQ writes consecutively
	// The inital value of waveForm already sets the Command up
	WriteControlRegister();			// Update control register properly
	WriteRegister(lower14);			// Write lower 14 bits to AD9833
	WriteRegister(upper14);			// Write upper 14 bits to AD9833
}

void AD9833 :: IncrementFrequency ( Registers freqReg, float freqIncHz ) {
	// Add/subtract a value from the current frequency programmed in
	// freqReg by the amount given
	float frequency = (freqReg == REG0) ? frequency0 : frequency1;
	SetFrequency(freqReg,frequency+freqIncHz);
}

void AD9833 :: SetPhase ( Registers phaseReg, float phaseInDeg ) {
	// Individual writes.  Phase is 12 LSB bits
	// The output signal will be phase shifted by 2π/4096 x PHASEREG
	// Sanity checks on input
	phaseInDeg = fmod(phaseInDeg,360);
	if ( phaseInDeg < 0 ) phaseInDeg += 360;
	
	// Phase is in float degress ( 0.0 - 360.0 )
	// Convert to a number 0 to 4096 where 4096 = 0 by masking
	uint16_t phaseVal = (uint16_t)(BITS_PER_DEG * phaseInDeg) & 0x0FFF;
	phaseVal |= PHASE_WRITE_CMD;
	if ( phaseReg == REG0 )	{
		phase0 = phaseInDeg;
	}
	else {
		phase1 = phaseInDeg;
		phaseVal |= PHASE1_WRITE_REG;
	}
	WriteRegister(phaseVal);	// OK, this is not a Control Write
}

void AD9833 :: IncrementPhase ( Registers phaseReg, float phaseIncDeg ) {
	// Add/subtract a value from the current phase programmed in
	// phaseReg by the amount given
	float phase = (phaseReg == REG0) ? phase0 : phase1;
	SetPhase(phaseReg,phase + phaseIncDeg);
}

void AD9833 :: SetWaveform ( WaveformType waveType ) {
	// Add error checking?
	waveForm = waveType;
	WriteControlRegister();
}

void AD9833 :: EnableOutput ( bool enable ) {
	// How to implement this? Hold in RESET while enable = false
	outputEnabled = enable;
	WriteControlRegister();
}

void AD9833 :: SetOutputSource ( Registers freqReg, Registers phaseReg ) {
	// Add more error checking?
	activeFreq = freqReg;
	if ( phaseReg == SAME_AS_REG0 )	activePhase = activeFreq;
	else activePhase = phaseReg;
	WriteControlRegister();
}

void AD9833 :: SleepMode ( bool enable ) {
	sleepEnabled = enable;
	WriteControlRegister();	
}

void AD9833 :: WriteControlRegister ( void ) {
	/*
	 * Why is SINE_WAVE a special case?
	 * SINE_WAVE = 0x2000, TRIANGLE_WAVE = 0x2002, SQUARE_WAVE = 0x2068,
	 * HALF_SQUARE_WAVE = 0x2060
	 * Control write, RESET disabled, Set Waveform type, set output reg
	 * of Frequency to 0 or 1, and Phase 0 or 1
	 * If a square wave, the DAC is turned off.
	 * Internal clock source is always enabled? Maybe allow a change?
	*/
	if ( activeFreq == REG0 )
		waveForm &= ~FREQ1_OUTPUT_REG;
	else
		waveForm |= FREQ1_OUTPUT_REG;
	if ( activePhase == REG0 )
		waveForm &= ~PHASE1_OUTPUT_REG;
	else
		waveForm |= PHASE1_OUTPUT_REG;
	if ( outputEnabled )
		waveForm &= ~RESET_CMD;
	else
		waveForm |= RESET_CMD;
	if ( sleepEnabled )
		waveForm |= SLEEP_MODE;
	else
		waveForm &= ~SLEEP_MODE;
	WriteRegister ( waveForm );
}

void AD9833 :: WriteRegister ( int16_t dat ) {
	/*
	 * We set the mode here, because other hardware may be doing SPI also
	 */
	SPI.setDataMode(SPI_MODE2);
  
	digitalWrite(FNCpin, LOW);		// FNCpin low to write to AD9833
	delayMicroseconds(10);			// delay a bit - is this too much?
  
	SPI.transfer(highByte(dat));	// Transmit 16 bits 8 bits at a time
	SPI.transfer(lowByte(dat));

	digitalWrite(FNCpin, HIGH);		// Write done.
}

