/*
 * AD9833.h
 * 
 * Copyright 2016 Bill Williams <wlwilliams1952@gmail.com, github/BillWilliams1952>
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
 * 
 */

#ifndef __AD9833__

#define __AD9833__

#include <Arduino.h>
#include <SPI.h>

#define pow2_28				268435456L	// 2^28 used in frequency word calculation
#define BITS_PER_DEG		11.3777777777778	// 4096 / 360

#define RESET_CMD			0x0100		// Control write, reset enabled
/*		Sleep mode
 * D7	1 = internal clock is disabled
 * D6	1 = put DAC to sleep
 */
#define SLEEP_MODE			0x00C0
#define PHASE_WRITE_CMD		0xC000
#define PHASE1_WRITE_REG	0x2000
#define FREQ0_WRITE_REG		0x4000
#define FREQ1_WRITE_REG		0x8000
#define PHASE1_OUTPUT_REG	0x0400		// Output is based off REG0/REG1
#define FREQ1_OUTPUT_REG	0x0800		// ditto

/*
 * Initial settings for the control register for waveform type.
 * Control write (D15 D14 = 00)
 * 		Output using FREQ0 for frequency and PHASE0 for phase
 * 		Reset disabled
 * 		DAC output active (SINE/TRIANGLE)/in sleep (SQUARE) based on waveform
 * 		Internal clock enabled.
 * 		Either a SINE, TRIANGLE, SQUARE, or HALF_SQUARE waveform
 * There seems to be a bug here.  I would assume that a control write does
 * not need Bit D13 set.  This seems to be true for ALL waveforms except
 * SINE_WAVE.  Why, I don't know.
 */
typedef enum { SINE_WAVE = 0x2000, TRIANGLE_WAVE = 0x2002,
			   SQUARE_WAVE = 0x2068, HALF_SQUARE_WAVE = 0x2060 } WaveformType;
			   
typedef enum { REG0, REG1, SAME_AS_REG0 } Registers;

class AD9833 {

public:
	
	AD9833 ( uint8_t FNCpin, uint32_t referenceFrequency = 25000000UL );

	// Must be the first command after creating the AD9833 object.
	void Begin ( void );

	// Reset counting registers, output is off
	// ANY function call after this removes the RESET condition
	void Reset ( void );

	// Update just the frequency in REG0 or REG1
	void SetFrequency ( Registers freqReg, float frequency );

	// Increment the selected frequency register by freqIncHz
	void IncrementFrequency ( Registers freqReg, float freqIncHz );

	// Update just the phase in REG0 or REG1
	void SetPhase ( Registers phaseReg, float phaseInDeg );

	// Increment the selected phase register by phaseIncDeg
	void IncrementPhase ( Registers phaseReg, float phaseIncDeg );

	// Set the output waveform for the selected frequency register
	// SINE_WAVE, TRIANGLE_WAVE, SQUARE_WAVE, HALF_SQUARE_WAVE,
	void SetWaveform ( Registers waveFormReg, WaveformType waveType );

	// Output based on the contents of REG0 or REG1
	void SetOutputSource ( Registers freqReg, Registers phaseReg = SAME_AS_REG0 );

	// Turn ON / OFF output using the RESET command.
	void EnableOutput ( bool enable );

	// Enable/disable Sleep mode.  Internal clock and DAC disabled
	void SleepMode ( bool enable );

	// TODO:
	void EnableDAC ( bool enable );

	// TODO
	void EnableInternalClock ( bool enable );

	// Return actual frequency programmed in register 
	float GetActualProgrammedFrequency ( Registers reg );

	// TODO Return actual phase programmed in register
	float GetActualProgrammedPhase ( Registers reg );

	// Return frequency resolution 
	float GetResolution ( void );

	// TODO: Setup everything at once
	void SetupSignal ( Registers freqReg, float frequency, Registers phaseReg,
						float phase, WaveformType waveType );

private:

	void 			WriteRegister ( int16_t dat );
	void 			WriteControlRegister ( void );
	uint16_t		waveForm0, waveForm1;
	uint8_t			FNCpin, outputEnabled, sleepEnabled;
	uint32_t		refFrequency;
	float			frequency0, frequency1, phase0, phase1;
	Registers		activeFreq, activePhase;
};

#endif

