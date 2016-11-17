# AD9833-Library-Arduino

## Synopsis
Library to control the AD9833 waveform generator 

This library allows the user to 

## Installation

Download the ZIP file and extract into your **sketchbook/libraries** directory. Exit the Arduino program (if open) and restart it to see the **AD9833** library along with its sketch examples.

The AD9833 uses SPI for communication. The following connections are required:

| AD9833 Pin | Arduino Pin | Description |
| :--------- | :---------- | :---------------------------------------- |
| **CLK** | **SCK** | SPI Clock pin |
| **DAT** | **MOSI** | SPI Master Out Slave In data pin |
| **FNC** | User defined | SPI transfer enable (active LOW) |
| **VCC** | **5V** | Recommend a 10 uF capacitor in parallel with a 0.1 uF capacitor connected between this pin and ground.|
| **GND** | **GND** | see comment above |

## API Reference

	AD9833 ( uint8_t FNCpin, uint32_t referenceFrequency = 25000000UL );

	// Must be the first command after creating the AD9833 object.
	void Begin ( void );

	// Reset counting registers, output is off
	void Reset ( void );

	// Update just the frequency in REG0 or REG1
	void SetFrequency ( Registers freqReg, float frequency );

	// Increment the selected frequency register by freqIncHz
	void IncrementFrequency ( Registers freqReg, float freqIncHz );

	// Update just the phase in REG0 or REG1
	void SetPhase ( Registers phaseReg, float phaseInDeg );

	// Increment the selected phase register by phaseIncDeg
	void IncrementPhase ( Registers phaseReg, float phaseIncDeg );

	// SINE_WAVE, TRIANGLE_WAVE, SQUARE_WAVE, HALF_SQUARE_WAVE,
	void SetWaveform ( WaveformType waveType );

	// Output based on the contents of REG0 or REG1
	void SetOutputSource ( Registers freqReg, Registers phaseReg = SAME_AS_REG0 );

	// Turn ON / OFF output using the RESET command
	void EnableOutput ( bool enable );

	// Enable/disable Sleep mode.  Internal clock and DAC disabled
	void SleepMode ( bool enable );

	// TODO: Setup everything at once
	void SetupSignal ( Registers freqReg, float frequency, Registers phaseReg,
						float phase, WaveformType waveType );

This program uses the Arduino API (**Arduino.h** and **spi.h**); no other special libraries are required. It has been tested on the Arduino Micro.

## Tests

Use the **AD9833_test_suite** example sketch to verify correct operation. Note that an oscilloscope and / or a spectrum analzer are required to completely verify correct operation.

## License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see http://www.gnu.org/licenses/.
