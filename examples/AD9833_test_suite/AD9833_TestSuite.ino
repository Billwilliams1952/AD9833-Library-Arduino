/*
 * AD9833_test_suite.ino
 * 2016 WLWilliams
 * 
 * This sketch demonstrates the use of the AD9833 DDS module library.
 * 
 * If you don't have an oscilloscope or spectrum analyzer, I don't quite know how you will
 * verify correct operation for some of the functions.
 * TODO: Add tests where the Arduino itself vereifies AD9833 basic operation.  Frequency of
 * square wave, sine/triangular wave using the A/D inputs (would need a level shifter).
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


#include <AD9833.h>        

#define RUNNING       F("\tRUNNING")
#define NOT_RUNNING   F("")
#define ON            F("ON")
#define OFF           F("OFF")
#define LED_PIN       13      // I'm alive blinker  
#define FNC_PIN       4       // Any digital pin. Used to enable SPI transfers (active LO  

// Some macros to 'improve' readability
#define BLINK_LED         digitalWrite(LED_PIN,millis()%1000 > 500);

/*
 * We need to manually call serialEventRun since we're not returning through the loop()
 * function while inside the test functions. If a character is in the receive buffer,
 * exit the test function. We also blink the I'm Alive LED to give a visual indication
 * that the program is not hung up.
 */
#define YIELD_ON_CHAR     if ( serialEventRun ) serialEventRun(); \
                          if ( Serial.available() ) return; \
                          BLINK_LED

#define DELAY_WITH_YIELD  for ( uint8_t i = 0; i < 10; i++ ) { \
                              YIELD_ON_CHAR \
                              delay(100);   \
                          }

#define FLUSH_SERIAL_INPUT  if ( serialEventRun ) serialEventRun(); \
                            do { Serial.read(); delay(100); } while ( Serial.available() > 0 );

//--------------- Create an AD9833 object ---------------- 
// Note, SCK and MOSI must be connected to CLK and DAT pins on the AD9833 for SPI
// -----      AD9833 ( FNCpin, referenceFrequency = 25000000UL )
AD9833 gen(FNC_PIN);       // Defaults to 25MHz internal reference frequency

void setup() { 
    pinMode(LED_PIN,OUTPUT);

    while (!Serial);          // Delay until terminal opens
    Serial.begin(9600);

    // This MUST be the first command after declaring the AD9833 object
    gen.Begin();              // The loaded defaults are 1000 Hz SINE_WAVE using REG0
                              // The output is OFF, Sleep mode is disabled
    gen.EnableOutput(false);  // Turn ON the output

    PrintMenu(0,true);        // Display menu for the first time
}

void loop() { 
    static bool outputOn = false;

    BLINK_LED

    if ( Serial.available() ) {
        char ch = Serial.read();

        FLUSH_SERIAL_INPUT
        
        PrintMenu(ch,outputOn);
        
        switch ( ch ) {
            case '1':
                IncrementFrequencyTest();
                break;
            case '2':
                CycleWaveformsTest();
                break;  
            case '3':
                SwitchFrequencyRegisterTest();
                break; 
            case '4':
                PhaseTest();
                break;
            case '5':
                RequestedvsProgrammedValues();
                break;
            case '6':
                outputOn = ! outputOn;
                gen.EnableOutput(outputOn);    // Turn off output
                break;
            default:
                Serial.println(F("*** Invalid command ***"));
                break;                    
        }      
    }
}

/*
 * Setup a manual ramp from a Start frequency to a Stop frequency in some increment
 * over a ramp time. 
 */
void IncrementFrequencyTest ( void ) {

    float startHz = 1000, stopHz = 5000, incHz = 1, sweepTimeSec = 5.0;
 
    // Calculate the delay between each increment.
    uint16_t numMsecPerStep = (sweepTimeSec * 1000.0) / ((uint16_t)((stopHz - startHz) / incHz) + 1);
    if ( numMsecPerStep == 0 ) numMsecPerStep = 1;

    // Apply a signal to the output. If phaseReg is not supplied, then
    // a phase of 0.0 is applied to the same register as freqReg
    gen.ApplySignal(SINE_WAVE,REG1,startHz);

    while ( true ) {
      
        gen.SetFrequency(REG1,startHz-incHz);

        for ( float i = startHz ; i <= stopHz; i += incHz ) {
            YIELD_ON_CHAR
            gen.IncrementFrequency(REG1,incHz);
            delay(numMsecPerStep); 
        }
    }
}

/*
 * Cycle through all of the waveform types. Also cycle the 
 * frequency registers.
 */
void CycleWaveformsTest ( void ) {
  
    WaveformType waveType = SINE_WAVE;
    gen.SetFrequency(REG0,10000.0);   // Load values
    gen.SetFrequency(REG1,1000.0);
    // We don't care about phase for this test
    
    while ( true ) {

        gen.SetWaveform(REG1,waveType);   // Next waveform
        gen.SetWaveform(REG0,waveType);
        gen.SetOutputSource(REG1);        // Output 1000 Hz waveform

        // Hack to allow I'm alive lamp a chance to blink and give a better
        // response to user input
        DELAY_WITH_YIELD
        
        gen.SetOutputSource(REG0);        // Output 10000 Hz waveform
        
        DELAY_WITH_YIELD

        switch ( waveType ) {             // Cycle through all the waveform types
            case SINE_WAVE:
                waveType = TRIANGLE_WAVE;
                break;
            case TRIANGLE_WAVE:
                waveType = SQUARE_WAVE;
                break;
            case SQUARE_WAVE:
                waveType = HALF_SQUARE_WAVE;
                break;
            case HALF_SQUARE_WAVE:
                waveType = SINE_WAVE;
                break; 
        }
    }    
}

/*
 * Fast switching example.
 * I use the FFT display capability on my scope
 */
void SwitchFrequencyRegisterTest ( void ) {

    gen.ApplySignal(SINE_WAVE,REG0,500000);
    gen.ApplySignal(SINE_WAVE,REG1,100000);
    gen.SetPhase(REG1,180);           // Offset second freq by 180 deg
    gen.Reset();

    while ( true ) {                  // This takes time
        
        YIELD_ON_CHAR                 // This takes more time

        gen.SetOutputSource(REG0);    // This takes about 18 usec
        gen.SetOutputSource(REG1);    // This takes about 18 usec  
        
        // What ends up is REG0 frequency is active a shorter amount of time
        // then REG1 frequency. In the sepctrum, the duty cycle differences will
        // show up (power is lower by 10log(DC))
    }  
}

/*
 * Phase shift between REG0 and REG1. Use a oscilloscope set to Normal
 * triggering, AC coupling, 500usec/div, 100 mV/div. This will display
 * about two cycles for register 0, 4 cycle for register 1, plus dead 
 * time for the Reset.
 * Use Normal triggering so the display remains even when triggering is 
 * lost. Can use any waveform for this test. Remember that the square 
 * wave is about 5v-pp while sine and triangle are about 600 mv-pp
 */
void PhaseTest ( void ) {

    gen.ApplySignal(TRIANGLE_WAVE,REG0,1000);
    gen.ApplySignal(SINE_WAVE,REG1,2000);

    bool reverse = true;

    while ( true ) {
        reverse = ! reverse;

        for ( int16_t i = 0; i <= 360; i += 1 ) {
            if ( ! reverse )
                gen.IncrementPhase(REG1,-1);
            else
                gen.IncrementPhase(REG1,1);

            YIELD_ON_CHAR
            /*
             * Display ~ 2 cycles using REG0 phase. If no REG is supplied for phase,
             * defaults to REG specified for frequency. RESET is removed during this
             * function call.
             */
            gen.SetOutputSource(REG0); 
            /*
             * This is just a wag to try to get exactly 2 cycles of the waveform. 
             * It makes the phase alignments easier to verify.
             */
            delayMicroseconds(1900);
                
            YIELD_ON_CHAR
            
            /* This also works if you keep using REG1 for frequency
             * Now display ~ 4 cycles using REG1
             */
            gen.SetOutputSource(REG1);
            delayMicroseconds(1950);
            /*
             * Turn off for remaining trace. Reset the registers so triggering occurs 
             * on the start of REG0 signal. Reset() includes 15 msec delay which is good  
             * to ensure sweep is completed. 
             * I tried using EnableOutput(true) then EnableOutput(false) in this
             * loop but could not get reliable triggering on the scope.
             * 
             * The difference between Reset() and EnableOutput(false) is that EnableOutput(false)
             * keeps the AD9833 in RESET until you specifically remove the RESET using 
             * EnableOutput(true). However, after a call to Reset(), calls to ANY function 
             * EXCEPT Set/Increment Phase will also remove the RESET.
             * 
             */
            gen.Reset(); 

            if ( i % 90 == 0  )
                delay(1000);    // Stop and show phase alignment between REG0 REG1
        }
    }
}

/*
 * Show the requested versus actual programmed values for frequency and phase
 * Also show resolution, max frequency (based on refFrequency)
 */
void RequestedvsProgrammedValues ( void ) {
  
    float requestedFrequency, programmedFrequency;
    char  buffer[20];   // 14 characters actually needed for display    

    gen.ApplySignal(SINE_WAVE,REG0,1000.0);
    
    while ( true ) {
      
        FLUSH_SERIAL_INPUT
  
        Serial.println(F("\nEnter frequency ('Q' to quit) >"));
        while ( !Serial.available() )   BLINK_LED

        if ( toupper(Serial.peek()) == 'Q' ) {
            // Need an extra <CR> ?
            FLUSH_SERIAL_INPUT    // why isn't this flushing input?
            return;
        }
        requestedFrequency = Serial.parseFloat();
        gen.SetFrequency(REG0,requestedFrequency);
        programmedFrequency = gen.GetActualProgrammedFrequency(REG0);
        Serial.print(F("Requested :"));
        dtostrf(requestedFrequency,14,5,buffer); 
        Serial.print(buffer);
        Serial.print(F("   Actual :"));
        dtostrf(programmedFrequency,14,5,buffer); 
        Serial.println(buffer);       
    }
}

/* 
 * Display the command menu
 */
void PrintMenu ( char ch, bool outputOn ) {
    Serial.println(); Serial.println();
    Serial.println(F("****** AD9833 Test Menu ******\n"));
    Serial.print(F("'1' IncrementFrequencyTest"));
    Serial.println(ch == '1' ? RUNNING : NOT_RUNNING);
    Serial.print(F("'2' CycleWaveformsTest\t"));
    Serial.println(ch == '2' ? RUNNING : NOT_RUNNING);
    Serial.print(F("'3' SwitchFrequencyRegisterTest"));
    Serial.println(ch == '3' ? RUNNING : NOT_RUNNING);
    Serial.print(F("'4' PhaseTest\t\t"));
    Serial.println(ch == '4' ? RUNNING : NOT_RUNNING);
    Serial.print(F("'5' RequestedvsProgrammedValues"));
    Serial.println(ch == '5' ? RUNNING : NOT_RUNNING);
    Serial.print(F("'6' Output "));  
    if ( ch == '6' ) {  
        if ( outputOn ) Serial.println(OFF);
        else            Serial.println(ON);
    }
    else {
        if ( outputOn ) Serial.println(ON);
        else            Serial.println(OFF);      
    }
    Serial.println(F("Enter a number 1 to 6 >"));
}



