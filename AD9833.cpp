/*
 * AD9833.cpp
 * 
 * Copyright 2016 Bill Williams <wlwilliams1952@gmail.com, github/BillWilliams1952>
 *
 * Thanks to john@vwlowen.co.uk for his work on the AD9833. His web page
 * is: http://www.vwlowen.co.uk/arduino/AD9833-waveform-generator/AD9833-waveform-generator.htm
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

/*
 * Create an AD9833 object
 */
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
	waveForm0 = waveForm1 = SINE_WAVE;
	frequency0 = frequency1 = 1000;		// 1 KHz sine wave to start
	phase0 = phase1 = 0.0;				// 0 phase
	activeFreq = REG0; activePhase = REG0;
}

/*
 * This MUST be the first command after declaring the AD9833 object
 * Start SPI and place the AD9833 in the RESET state
 */
void AD9833 :: Begin ( void ) {
	SPI.begin();
	delay(100);
	Reset();
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

/*
 * Hold the AD9833 in RESET state until the next command of any type.
 * Reset = 1 resets internal registers to 0, which corresponds to an
 * analog output of midscale - digital output at 0.
 * Reset = 0 disables reset. 
 * 
 * The difference between Reset() and EnableOutput(false) is that
 * EnableOutput(false) keeps the AD9833 in the RESET state until you
 * specifically remove the RESET state using EnableOutput(true).
 * With a call to Reset(), ANY subsequent call to ANY function (other
 * than Reset itself) will also remove the RESET state.
 */
void AD9833 :: Reset ( void ) {
	WriteRegister(RESET_CMD);
	delay(15);
}

/*
 *  Set the specified frequency register with the frequency (in Hz)
 */
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

	// I do not reset the registers during write. It seems to remove
	// 'glitching' on the outputs.
	WriteControlRegister();
	// Control register has already been setup to accept two frequency
	// writes, one for each 14 bit part of the 28 bit frequency word
	WriteRegister(lower14);			// Write lower 14 bits to AD9833
	WriteRegister(upper14);			// Write upper 14 bits to AD9833
}

/*
 * Increment the specified frequency register with the frequency (in Hz)
 */
void AD9833 :: IncrementFrequency ( Registers freqReg, float freqIncHz ) {
	// Add/subtract a value from the current frequency programmed in
	// freqReg by the amount given
	float frequency = (freqReg == REG0) ? frequency0 : frequency1;
	SetFrequency(freqReg,frequency+freqIncHz);
}

/*
 *  Set the specified phase register with the phase (in degrees)
 */
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
	WriteRegister(phaseVal);
}

/*
 * Increment the specified phase register by the phase (in degrees)
 */
void AD9833 :: IncrementPhase ( Registers phaseReg, float phaseIncDeg ) {
	// Add/subtract a value from the current phase programmed in
	// phaseReg by the amount given
	float phase = (phaseReg == REG0) ? phase0 : phase1;
	SetPhase(phaseReg,phase + phaseIncDeg);
}

/*
 * Set the type of waveform that is output for a frequency register
 * SINE_WAVE, TRIANGLE_WAVE, SQUARE_WAVE, HALF_SQUARE_WAVE
 */
void AD9833 :: SetWaveform (  Registers waveFormReg, WaveformType waveType ) {
	if ( waveFormReg == REG0 )
		waveForm0 = waveType;
	else
		waveForm1 = waveType;
	WriteControlRegister();
}

/*
 * EnableOutput(false) keeps the AD9833 is RESET state until a call to
 * EnableOutput(true). See the Reset function description
 */
void AD9833 :: EnableOutput ( bool enable ) {
	// How to implement this? Hold in RESET while enable = false
	outputEnabled = enable;
	WriteControlRegister();
}

/*
 * Set which frequency and phase register is being used to output the
 * waveform. If phaseReg is not supplied, it defaults to the same
 * register as freqReg.
 */
void AD9833 :: SetOutputSource ( Registers freqReg, Registers phaseReg ) {
	// Add more error checking?
	activeFreq = freqReg;
	if ( phaseReg == SAME_AS_REG0 )	activePhase = activeFreq;
	else activePhase = phaseReg;
	WriteControlRegister();
}

//---------- LOWER LEVEL FUNCTIONS NOT NORMALLY NEEDED -------------

/*
 * Disable/enable both the internal clock and the DAC. Note that square
 * wave outputs are avaiable if using an external Reference. ??? IS THIS
 * TRUE ??
 */
void AD9833 :: SleepMode ( bool enable ) {
	// TODO: Call EnableDAC(enable) and EnableInternalClock(enable)
	sleepEnabled = enable;
	WriteControlRegister();	
}

/*
 * This enables / disables the DAC. It will override any previous DAC
 * setting by Waveform type, or via the SleepMode function
 */
void AD9833 :: EnableDAC ( bool enable ) {
	
}

/*
 * This enables / disables the internal clock. It will override any 
 * previous clock setting by the SleepMode function
 */
void AD9833 :: EnableInternalClock ( bool enable ) { 
	
}

// ------------ STATUS / INFORMATION FUNCTIONS -------------------
/*
 * Return actual frequency programmed
 */
float AD9833 :: GetActualProgrammedFrequency ( Registers reg ) {
	float frequency = reg == REG0 ? frequency0 : frequency1;
	int32_t freqWord = (uint32_t)((frequency * pow2_28) / (float)refFrequency) & 0x0FFFFFFFUL;
	return (float)freqWord * (float)refFrequency / (float)pow2_28;
}

/*
 * TODO
 * Return actual phase programmed
 */
float AD9833 :: GetActualProgrammedPhase ( Registers reg ) {
	return 0.0;
}

/*
 * Return frequency resolution
 */
float AD9833 :: GetResolution ( void ) {
	return (float)refFrequency / (float)pow2_28;
}

// --------------------- PRIVATE FUNCTIONS --------------------------

void AD9833 :: WriteControlRegister ( void ) {
	/*
	 * Control write, RESET disabled, Set Waveform type, set output reg
	 * of Frequency to 0 or 1, and Phase 0 or 1.
	 * If a square wave, the DAC is turned off.
	 * Internal clock source is always enabled? Maybe allow a change?
	*/
	uint16_t waveForm; 
	if ( activeFreq == REG0 ) {
		waveForm = waveForm0;
		waveForm &= ~FREQ1_OUTPUT_REG;
	}
	else {
		waveForm = waveForm1;
		waveForm |= FREQ1_OUTPUT_REG;
	}
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
	// TODO: Now check EnableDAC flag and EnableInternalClock bit
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

