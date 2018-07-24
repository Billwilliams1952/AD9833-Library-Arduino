# AD9833-Library-Arduino

## Synopsis
Library to control the AD9833 DDS waveform generator. The library allows the user to independently program frequency, phase, and waveform type for both registers.

**From Analog Devices data sheet:** (http://www.analog.com/media/en/technical-documentation/data-sheets/AD9833.pdf)
>"The AD9833 is a low power, programmable waveform generator capable of producing sine, triangular, and square wave outputs. Waveform generation is required in various types of sensing, actuation, and time domain reflectometry (TDR) applications. The output frequency and phase are software programmable, allowing easy tuning. No external components are needed. The frequency registers are 28 bits wide: with a 25 MHz clock rate, resolution of 0.1 Hz can be achieved; with a 1 MHz clock rate, the AD9833 can be tuned to 0.004 Hz resolution. The AD9833 is written to via a 3-wire serial interface. This serial interface operates at clock rates up to 40 MHz and is compatible with DSP and microcontroller standards. The device operates with a power supply from 2.3 V to 5.5 V."

## Updates

| Version | Date | Description |
| :--------- | :---------- | :---------------------------------------- |
| - | - | Initial Release |
|   | 6/2/2018 |  Added simple ApplySignal.ino file to examples directory               |


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
```C++
AD9833 ( uint8_t FNCpin, uint32_t referenceFrequency = 25000000UL );

// Must be the first command after creating the AD9833 object.
void Begin ( void );

// The difference between Reset() and EnableOutput(false) is that
// EnableOutput(false) keeps the AD9833 in the RESET state until you
// specifically remove the RESET state using EnableOutput(true).
// With a call to Reset(), ANY subsequent call to ANY function (other
// than Reset itself and Set/IncrementPhase) will also remove the
// RESET state.
void Reset ( void );
	
// Setup and apply a signal. Note that any calls to EnableOut,
// SleepMode, DisableDAC, or DisableInternalClock remain in effect
void ApplySignal ( WaveformType waveType, Registers freqReg, float frequencyInHz,
		Registers phaseReg = SAME_AS_REG0, float phaseInDeg = 0.0  );

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

// Enable / Disable DAC
void EnableDAC ( bool enable );

// Enable / Disable Internal Clock
void EnableInternalClock ( bool enable );

// Return actual frequency programmed in register 
float GetActualProgrammedFrequency ( Registers reg );

// Return actual phase programmed in register
float GetActualProgrammedPhase ( Registers reg );

// Return frequency resolution 
float GetResolution ( void );
```
This program uses the Arduino API (**Arduino.h** and **spi.h**); no other special libraries are required. It has been tested on the Arduino Micro.

## Tests

Use the **AD9833_test_suite** example sketch to verify correct operation. Note that an oscilloscope and / or a spectrum analzer are required to completely verify correct operation.

![alt tag](https://cloud.githubusercontent.com/assets/3778024/20465143/4108022e-af1c-11e6-96e9-26b73d52e730.png)

![alt tag](https://cloud.githubusercontent.com/assets/3778024/20465125/011e6694-af1c-11e6-8f17-655415a0de87.png)

## License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see http://www.gnu.org/licenses/.
