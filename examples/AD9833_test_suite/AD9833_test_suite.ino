/*
 * AD9833Test.ino
 * 2016 WLWilliams
 * 
 * This sketch demonstrates the use of the AD9833 DDS module library.
 * 
 * If you don't have an oscilloscope or spectrum analyzer, I don't quite know how you will
 * verify correct operation for some of the functions.
 * 
 * This program is free software: you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version. 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of
 * the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * Library originally added November 13, 2016 by Bill Williams
 *
 * This example code is in the public domain.
 * 
 * TODO:  Add phase register test. How to do this?
 * TODO:  ADD GITHUB LINK
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
                          
// AD9833 ( FNCpin, referenceFrequency = 25000000UL )
AD9833 gen(FNCpin);       // Defaults to 25MHz internal reference frequency

void setup() { 
    pinMode(LED_PIN,OUTPUT);

    Serial.begin(9600);
    delay(5000);              // Time to open Serial Window to see initial menu

    // This MUST be the first command after declaring the AD9833 object
    gen.Begin();              // The loaded defaults are 1000 Hz SINE_WAVE using REG0
                              // The output is OFF
    gen.EnableOutput(false);  // Turn on the output

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
    gen.SetWaveform(SINE_WAVE);

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
    gen.SetFrequency(REG1,1000.0);1
    
    while ( true ) {

        gen.SetWaveform(waveType);

        gen.SetOutputSource(REG1);    // Output 1000 Hz waveform

        // Hack to allow I'm alive lamp a chance to blink
        for ( int8_t i = 0; i < 10; i++ ) {
            delay(100);                   // Up to a 1 second 'lag' in menu response
            YIELD_ON_CHAR
        }
        
        gen.SetOutputSource(REG0);    // Output 10000 Hz waveform

        // Hack to allow I'm alive lamp a chance to blink
        for ( int8_t i = 0; i < 10; i++ ) {
            delay(100);                   // Up to a 1 second 'lag' in menu response
            YIELD_ON_CHAR
        }

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
 */
void SwitchFrequencyRegisterTest ( void ) {

    gen.SetWaveform(SINE_WAVE);
    gen.SetFrequency(REG0,50000.0);
    gen.SetFrequency(REG1,10000.0);

    while ( true ) {
        YIELD_ON_CHAR
        gen.SetOutputSource(REG1);
        delayMicroseconds(500);
        gen.SetOutputSource(REG0);
        delayMicroseconds(500);        
    }  
}

/* 
 * Display the oommand menu
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
    Serial.print("'4' Output ");  
    if ( ch == '4' ) {  
        if ( outputOn ) Serial.println("OFF");
        else            Serial.println("ON");
    }
    else {
        if ( outputOn ) Serial.println("ON");
        else            Serial.println("OFF");      
    }
    Serial.println("Enter a number 1 to 4 >");
}


