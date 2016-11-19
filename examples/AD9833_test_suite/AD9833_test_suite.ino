/*
 * AD9833Test.ino
 * 2016 WLWilliams
 * 
 * This sketch demonstrates the use of the AD9833 DDS module library.
 * 
 * If you don't have an oscilloscope or spectrum analyzer, I don't quite know how you will
 * verify correct operation for some of the functions.
 * TODO: Add tests where the Arduino itself vereifies AD9833 basic operation.  Frequency of
 * square wave, sinve/triangular wave using the A/D inputs.
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

#define LED_PIN   13      // I'm alive blinker
// Note, SCK and MOSI must be connected to CLK and DAT pins on the AD9833 for SPI
#define FNCpin    A5      // Any digital pin. Used to enable SPI transfers (active LOW)

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
                     
// AD9833 ( FNCpin, referenceFrequency = 25000000UL )
AD9833 gen(FNCpin);       // Defaults to 25MHz internal reference frequency

void setup() { 
    pinMode(LED_PIN,OUTPUT);

    Serial.begin(9600);
    delay(5000);              // Time to open Serial Window to see initial menu

    // This MUST be the first command after declaring the AD9833 object
    gen.Begin();              // The loaded defaults are 1000 Hz SINE_WAVE using REG0
                              // The output is OFF, Sleep mode is disabled
    gen.EnableOutput(false);  // Turn OFF the output

    PrintMenu(0,true);        // Display menu for the first time
}

void loop() { 
    static bool outputOn = false;

    BLINK_LED

    if ( Serial.available() ) {
        char ch = Serial.read();
        while ( Serial.available() ) Serial.read();   // Why is this needed?
        
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
                outputOn = ! outputOn;
                gen.EnableOutput(outputOn);    // Turn off output
                break;
            default:
                Serial.println("*** Invalid command ***");
                break;                    
        }      
    }
}

/*
 * Setup a manual ramp from a Start frequency to a Stop frequency in some increment
 * over a ramp time. 
 */
void IncrementFrequencyTest ( void ) {

    float startHz = 100, stopHz = 2000, incHz = 10, sweepTimeSec = 10.0;
 
    // Calculate the delay between each increment.
    uint16_t numMsecPerStep = (sweepTimeSec * 1000.0) / ((uint16_t)((stopHz - startHz) / incHz) + 1);
    if ( numMsecPerStep == 0 ) numMsecPerStep = 1;

    gen.SetOutputSource(REG1);    // Lets use REG1 for this example
    gen.SetWaveform(REG1,SINE_WAVE);
    // We don't care about phase for this test

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

        gen.SetWaveform(REG1,waveType);
        gen.SetWaveform(REG0,waveType);
        gen.SetOutputSource(REG1);    // Output 1000 Hz waveform

        // Hack to allow I'm alive lamp a chance to blink and give a better
        // response to user input
        DELAY_WITH_YIELD
        
        gen.SetOutputSource(REG0);    // Output 10000 Hz waveform
        
        DELAY_WITH_YIELD

        switch ( waveType ) {         // cycle through all the waveform types
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
 * Very fast switching example.
 * I use the FFT display capability on my scope Rigol DS1054Z.
 */
void SwitchFrequencyRegisterTest ( void ) {

    gen.SetWaveform(REG0,SINE_WAVE);
    gen.SetWaveform(REG1,SINE_WAVE);
    gen.SetFrequency(REG0,50000.0);
    gen.SetFrequency(REG1,10000.0);
    // We don't care about phase for this test

    while ( true ) {
      
        YIELD_ON_CHAR
        
        gen.SetOutputSource(REG1);
        delayMicroseconds(500);
        gen.SetOutputSource(REG0);
        delayMicroseconds(500);        
    }  
}

/*
 * Phase shift between REG0 and REG1. Use a oscilloscope set to Normal
 * triggering, AC coupling, 500usec/div, 100 mV/div. This will display
 * about two cycles for each register, plus dead time for the Reset.
 * Use Normal triggering so the display remains even when triggering is 
 * lost. Can use any waveform for this test. Remember that the square 
 * wave is about 5v-pp while sine and triangle are about 600 mv-pp
 */
void PhaseTest ( void ) {
  
    gen.SetWaveform(REG0,TRIANGLE_WAVE);   // Any waveform can work
    gen.SetFrequency(REG0,1000.0);    // 1 KHz
    gen.SetPhase(REG0,0.0);           // Phase is 0 for first register
    gen.SetFrequency(REG1,2000.0);    // 1 Khz
    gen.SetPhase(REG1,0.0);           // Phase is initially 0
    gen.SetWaveform(REG1,SINE_WAVE);   // Any waveform can work

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
             * Now display ~ 2 cycles using REG1
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
 * Allow the user to enter a frequency, select a waveform, and select what register
 * For this test, phase is not tested. Both phase registers are loaded with 0.0
 */
void ManualFrequencyTest ( void ) {
  
}

/*
 * Show the requested versus actual programmed values for frequency and phase
 * Also show resolution, max frequency (based on refFrequency)
 */
void RequestedvsProgrammedValues ( void ) {
  
}

/* 
 * Display the command menu
 */
void PrintMenu ( char ch, bool outputOn ) {
    Serial.println(); Serial.println();
    Serial.println("****** AD9833 Test Menu ******"); Serial.println();
    Serial.print("'1' IncrementFrequencyTest"); 
    if ( ch == '1' )  Serial.println(" RUNNING");
    else              Serial.println("");
    Serial.print("'2' CycleWaveformsTest");
    if ( ch == '2' )  Serial.println(" RUNNING");
    else              Serial.println("");
    Serial.print("'3' SwitchFrequencyRegisterTest");
    if ( ch == '3' )  Serial.println(" RUNNING");
    else              Serial.println("");
    Serial.print("'4' PhaseTest (Not done yet)");
    if ( ch == '4' )  Serial.println(" RUNNING");
    else              Serial.println("");
    Serial.print("'5' Output ");  
    if ( ch == '5' ) {  
        if ( outputOn ) Serial.println("OFF");
        else            Serial.println("ON");
    }
    else {
        if ( outputOn ) Serial.println("ON");
        else            Serial.println("OFF");      
    }
    Serial.println("Enter a number 1 to 5 >");
}

