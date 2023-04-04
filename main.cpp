/*! @mainpage Example 3.5 
 * @date Thursday, March 30, 2023
 * @authors Agustin de Vedia & Ulises Montenegro
 *
 * @file main.cpp Main program script. It's pretty much all here.
 *
 * @note TP03 Implemented in class.
 *
 * @brief
 * root/
 *  mbed-os                 : Mbed code to abstract and facilitate development.
 *  .gitignore              : Files to be ignored by Git.
 *  arm_book_lib.h          : Includes & definitions to help develop proyects from the book.
 *  compile_commands.json   : Compile commands.
 *  main.cpp                : Main program.
 *  mbed-os.lib             : Mbed repository.
 *
 */

//=====[Libraries]=============================================================

#include "mbed.h" ///< Mbed code to abstract and facilitate development.
#include "arm_book_lib.h" ///< Includes & definitions to help develop proyects from the book.
#include <stdio.h> ///< C standard library.
#include <string.h> ///< C String library.

//=====[Defines]===============================================================

#define NUMBER_OF_KEYS                          4 ///< Number of keys in the numerical keyboard.
#define BLINKING_TIME_GAS_ALARM                 1000 ///< Blinking frecuency for the gas alarm.
#define BLINKING_TIME_OVER_TEMP_ALARM           500 ///< Blinking frecuency for the over temperature alarm.
#define BLINKING_TIME_GAS_AND_OVER_TEMP_ALARM   100 ///< Blinking frecuency for the alarm when gas and overtemperature are detected simultaneously.
#define NUMBER_OF_AVG_SAMPLES                   100 ///< Number of samples to average when sensing temperature.
#define OVER_TEMP_LEVEL                         50 ///< Parameter to calibrate trigger temperature.
#define TIME_INCREMENT_MS                       10 ///< Time step for mainloop.

//=====[Declaration and initialization of public global objects]===============

// @note DigitalIn / DigitalOut classes analysed in 'Example 1.1'

/**
* 'Enter' button is board button BUTTON1.
*/
DigitalIn enterButton(BUTTON1);

/**
* Button to test alarm. Connected to pin D2.
*/
DigitalIn alarmTestButton(D2);

/**
* Keyboard A button connected to pin D4.
*/
DigitalIn aButton(D4);

/**
* Keyboard B button connected to pin D5.
*/
DigitalIn bButton(D5);

/**
* Keyboard C button connected to pin D6.
*/
DigitalIn cButton(D6);

/**
* Keyboard D button connected to pin D7.
*/
DigitalIn dButton(D7);

/**
* Gas sensor connected to pin PE_12.
*/
DigitalIn mq2(PE_12);

/**
* Alarm LED is board LED LED1.
*/
DigitalOut alarmLed(LED1);

/**
* 'Incorrect code' LED is board LED LED1.
*/
DigitalOut incorrectCodeLed(LED3);

/**
* 'System blocked' LED is board LED LED1.
*/
DigitalOut systemBlockedLed(LED2);

/**
* Siren connected to pin PE_10. To sound the alarm it is set as output. When the alarm is inactive, it is set as input.
* This is managed by the methods input() and output().
*/
DigitalInOut sirenPin(PE_10);

// @note Constructor defined in "/home/studio/workspace/example-3.5-tp_03/mbed-os/drivers/include/drivers/UnbufferedSerial.h"
/**
* Set UART communication.
* Constructor implemented in "/home/studio/workspace/example-3.5-tp_03/mbed-os/drivers/include/drivers/UnbufferedSerial.h".
* Default baudrate: 9600.
* UART methods used in main.cpp script:
*   readable()  : Determines if there is a character available to read (/home/studio/workspace/example-3.5-tp_03/mbed-os/drivers/include/drivers/SerialBase.h)
*   read()      : Method to read recieved n bytes and returns # of bytes read.
*   write()     : Method to transmit n bytes from a file and returns # of bytes writen.
*
* @note Diference between printf() and uartUsb::write()
*  printf()    : funcion writen in C which sends writes n bytes to a file. Implemented in "stdio.h" standard library.
*  write()     : UnbufferedSerial method writen in C++ which . Implemented in the UnbufferedSerial Class.
*/
UnbufferedSerial uartUsb(USBTX, USBRX, 115200); // Default baudrate: 9600

/**
* Potenciometer connected to analog pin A0.
* AnalogIn constructor defined in "/home/studio/workspace/example-3.5-tp_03/mbed-os/drivers/include/drivers/AnalogIn.h"
*/
AnalogIn potentiometer(A0);

/**
* LM35 temperature sensor connected to analog pin A1.
*/
AnalogIn lm35(A1);

//=====[Declaration and initialization of public global variables]=============

bool alarmState    = OFF; ///< Bool that indicates the alarm state.
bool incorrectCode = false; ///< Bool that indicates if the code entered is incorrect.
bool overTempDetector = OFF; ///< Bool that indicates if the LM35 sensor temperature is greater than the trigger temperature.

int numberOfIncorrectCodes = 0; ///< Number of times in a row an incorrect code has been entered. Once the correct code is entered, this resets to 0.
int buttonBeingCompared    = 0; ///< Indicates the button being compared.
int codeSequence[NUMBER_OF_KEYS]   = { 1, 1, 0, 0 }; ///< Secret code sequence.
int buttonsPressed[NUMBER_OF_KEYS] = { 0, 0, 0, 0 }; ///< Pressed buttons.
int accumulatedTimeAlarm = 0; ///< Time the alarm output has been ON. This is to keep track of the alarm frequency.

bool gasDetectorState          = OFF; ///< Bool that indicates if the gas is being detected.
bool overTempDetectorState     = OFF; ///< Bool that indicates if an over temperature is being detected.

float potentiometerReading = 0.0; ///< Potenciometer reading.
float lm35ReadingsAverage  = 0.0; ///< Average of the temperature readings in a small time interval.
float lm35ReadingsSum      = 0.0; ///< Sum of the temperature readings in a small time interval.
float lm35ReadingsArray[NUMBER_OF_AVG_SAMPLES]; ///< LM35 temperature readings in a small time interval.
float lm35TempC            = 0.0; ///< Temperature sensed in the Celsius scale.

//=====[Declarations (prototypes) of public functions]=========================

/**
* Initialices input parameters.
*/
void inputsInit();

/**
* Initialices output parameters.
*/
void outputsInit();

/**
* Updates the Alarm state to ON if necessary.
*/
void alarmActivationUpdate();

/**
* Updates the Alarm state to OFF if the code entered is correct.
*/
void alarmDeactivationUpdate();

/**
* Updates the UART communication. Sends & receives data.
*/
void uartTask();

/**
* Outputs trough UART comunication the available commands for the user.
*/
void availableCommands();

/**
* Checks entered code and secret code and outputs True or False accordingly.
* @param[out] areEqual boolean variable indicating if the codes are equal or not.
*/
bool areEqual();

/**
* Converts temperature from the Celsius scale to the Fahrenheit scale.
* @param[in] tempInCelsiusDegrees Temperature in the Celsius scale.
* @param[out] tempInFahrenheitDegrees Temperature in the Fahrenheit scale.
*/
float celsiusToFahrenheit( float tempInCelsiusDegrees );

/**
* Translates the analog reading from the LM35 sensor to a temperature value in the celsius scale.
* @param[in] analogReading Analog reading from the LM35 sensor.
* @param[out] tempInCelsiusDegrees Temperature in the Celsius scale.
*/
float analogReadingScaledWithTheLM35Formula( float analogReading );

//=====[Main function, the program entry point after power on or reset]========

/**
* Main program function.
*/
int main()
{
    inputsInit();
    outputsInit();
    while (true) {
        alarmActivationUpdate();
        alarmDeactivationUpdate();
        uartTask();
        delay(TIME_INCREMENT_MS);
    }
}

//=====[Implementations of public functions]===================================

void inputsInit()
{
    alarmTestButton.mode(PullDown);
    aButton.mode(PullDown);
    bButton.mode(PullDown);
    cButton.mode(PullDown);
    dButton.mode(PullDown);
    sirenPin.mode(OpenDrain);
    sirenPin.input();
}

void outputsInit()
{
    alarmLed = OFF;
    incorrectCodeLed = OFF;
    systemBlockedLed = OFF;
}

void alarmActivationUpdate()
{
    static int lm35SampleIndex = 0;
    int i = 0;

    lm35ReadingsArray[lm35SampleIndex] = lm35.read(); // @note Reads the input voltage using function analogin_read() (declared in /home/studio/workspace/example-3.5-tp_03/mbed-os/hal/include/hal/analogin_api.h)
    lm35SampleIndex++;
    if ( lm35SampleIndex >= NUMBER_OF_AVG_SAMPLES) {
        lm35SampleIndex = 0;
    }
    
       lm35ReadingsSum = 0.0;
    for (i = 0; i < NUMBER_OF_AVG_SAMPLES; i++) {
        lm35ReadingsSum = lm35ReadingsSum + lm35ReadingsArray[i];
    }
    lm35ReadingsAverage = lm35ReadingsSum / NUMBER_OF_AVG_SAMPLES;
       lm35TempC = analogReadingScaledWithTheLM35Formula ( lm35ReadingsAverage );    
    
    if ( lm35TempC > OVER_TEMP_LEVEL ) {
        overTempDetector = ON;
    } else {
        overTempDetector = OFF;
    }

    if( !mq2) {
        gasDetectorState = ON;
        alarmState = ON;
    }
    if( overTempDetector ) {
        overTempDetectorState = ON;
        alarmState = ON;
    }
    if( alarmTestButton ) {             
        overTempDetectorState = ON;
        gasDetectorState = ON;
        alarmState = ON;
    }    
    if( alarmState ) { 
        accumulatedTimeAlarm = accumulatedTimeAlarm + TIME_INCREMENT_MS;
        sirenPin.output();                                     
        sirenPin = LOW;                                        
    
        if( gasDetectorState && overTempDetectorState ) {
            if( accumulatedTimeAlarm >= BLINKING_TIME_GAS_AND_OVER_TEMP_ALARM ) {
                accumulatedTimeAlarm = 0;
                alarmLed = !alarmLed;
            }
        } else if( gasDetectorState ) {
            if( accumulatedTimeAlarm >= BLINKING_TIME_GAS_ALARM ) {
                accumulatedTimeAlarm = 0;
                alarmLed = !alarmLed;
            }
        } else if ( overTempDetectorState ) {
            if( accumulatedTimeAlarm >= BLINKING_TIME_OVER_TEMP_ALARM  ) {
                accumulatedTimeAlarm = 0;
                alarmLed = !alarmLed;
            }
        }
    } else{
        alarmLed = OFF;
        gasDetectorState = OFF;
        overTempDetectorState = OFF;
        sirenPin.input();                                  
    }
}

void alarmDeactivationUpdate()
{
    if ( numberOfIncorrectCodes < 5 ) {
        if ( aButton && bButton && cButton && dButton && !enterButton ) {
            incorrectCodeLed = OFF;
        }
        if ( enterButton && !incorrectCodeLed && alarmState ) {
            buttonsPressed[0] = aButton;
            buttonsPressed[1] = bButton;
            buttonsPressed[2] = cButton;
            buttonsPressed[3] = dButton;
            if ( areEqual() ) {
                alarmState = OFF;
                numberOfIncorrectCodes = 0;
            } else {
                incorrectCodeLed = ON;
                numberOfIncorrectCodes++;
            }
        }
    } else {
        systemBlockedLed = ON;
    }
}

void uartTask()
{
    printf("UART Task initiated.");
    char receivedChar = '\0';
    char str[100];
    int stringLength;
    if( uartUsb.readable() ) {
        uartUsb.read( &receivedChar, 1 );
        switch (receivedChar) {
        case '1':
            if ( alarmState ) {
                uartUsb.write( "The alarm is activated\r\n", 24);
            } else {
                uartUsb.write( "The alarm is not activated\r\n", 28);
            }
            break;

        case '2':
            if ( !mq2 ) {
                uartUsb.write( "Gas is being detected\r\n", 22);
            } else {
                uartUsb.write( "Gas is not being detected\r\n", 27);
            }
            break;

        case '3':
            if ( overTempDetector ) {
                uartUsb.write( "Temperature is above the maximum level\r\n", 40);
            } else {
                uartUsb.write( "Temperature is below the maximum level\r\n", 40);
            }
            break;
            
        case '4':
            uartUsb.write( "Please enter the code sequence.\r\n", 33 );
            uartUsb.write( "First enter 'A', then 'B', then 'C', and ", 41 ); 
            uartUsb.write( "finally 'D' button\r\n", 20 );
            uartUsb.write( "In each case type 1 for pressed or 0 for ", 41 );
            uartUsb.write( "not pressed\r\n", 13 );
            uartUsb.write( "For example, for 'A' = pressed, ", 32 );
            uartUsb.write( "'B' = pressed, 'C' = not pressed, ", 34);
            uartUsb.write( "'D' = not pressed, enter '1', then '1', ", 40 );
            uartUsb.write( "then '0', and finally '0'\r\n\r\n", 29 );

            incorrectCode = false;

            for ( buttonBeingCompared = 0;
                  buttonBeingCompared < NUMBER_OF_KEYS;
                  buttonBeingCompared++) {

                uartUsb.read( &receivedChar, 1 );
                uartUsb.write( "*", 1 );

                if ( receivedChar == '1' ) {
                    if ( codeSequence[buttonBeingCompared] != 1 ) {
                        incorrectCode = true;
                    }
                } else if ( receivedChar == '0' ) {
                    if ( codeSequence[buttonBeingCompared] != 0 ) {
                        incorrectCode = true;
                    }
                } else {
                    incorrectCode = true;
                }
            }

            if ( incorrectCode == false ) {
                uartUsb.write( "\r\nThe code is correct\r\n\r\n", 25 );
                alarmState = OFF;
                incorrectCodeLed = OFF;
                numberOfIncorrectCodes = 0;
            } else {
                uartUsb.write( "\r\nThe code is incorrect\r\n\r\n", 27 );
                incorrectCodeLed = ON;
                numberOfIncorrectCodes++;
            }                
            break;

        case '5':
            uartUsb.write( "Please enter new code sequence\r\n", 32 );
            uartUsb.write( "First enter 'A', then 'B', then 'C', and ", 41 );
            uartUsb.write( "finally 'D' button\r\n", 20 );
            uartUsb.write( "In each case type 1 for pressed or 0 for not ", 45 );
            uartUsb.write( "pressed\r\n", 9 );
            uartUsb.write( "For example, for 'A' = pressed, 'B' = pressed,", 46 );
            uartUsb.write( " 'C' = not pressed,", 19 );
            uartUsb.write( "'D' = not pressed, enter '1', then '1', ", 40 );
            uartUsb.write( "then '0', and finally '0'\r\n\r\n", 29 );

            for ( buttonBeingCompared = 0; 
                  buttonBeingCompared < NUMBER_OF_KEYS; 
                  buttonBeingCompared++) {

                uartUsb.read( &receivedChar, 1 );
                uartUsb.write( "*", 1 );

                if ( receivedChar == '1' ) {
                    codeSequence[buttonBeingCompared] = 1;
                } else if ( receivedChar == '0' ) {
                    codeSequence[buttonBeingCompared] = 0;
                }
            }

            uartUsb.write( "\r\nNew code generated\r\n\r\n", 24 );
            break;
 
        case 'p':
        case 'P':
            potentiometerReading = potentiometer.read();
            sprintf ( str, "Potentiometer: %.2f\r\n", potentiometerReading );
            stringLength = 100; // HARDCODEADO DV strlen(str);
            uartUsb.write( str, stringLength );
            break;

        case 'c':
        case 'C':
            sprintf ( str, "Temperature: %.2f \xB0 C\r\n", lm35TempC );
            stringLength = 100; // HARDCODEADO DV strlen(str);
            uartUsb.write( str, stringLength );
            break;

        case 'f':
        case 'F':
            sprintf ( str, "Temperature: %.2f \xB0 F\r\n", 
                celsiusToFahrenheit( lm35TempC ) );
            stringLength = 100; // HARDCODEADO DV strlen(str);
            uartUsb.write( str, stringLength );
            break;

        default:
            availableCommands();
            break;

        }
    }
}

void availableCommands()
{
    uartUsb.write( "Available commands:\r\n", 21 );
    uartUsb.write( "Press '1' to get the alarm state\r\n", 34 );
    uartUsb.write( "Press '2' to get the gas detector state\r\n", 41 );
    uartUsb.write( "Press '3' to get the over temperature detector state\r\n", 54 );
    uartUsb.write( "Press '4' to enter the code sequence\r\n", 38 );
    uartUsb.write( "Press '5' to enter a new code\r\n", 31 );
    uartUsb.write( "Press 'P' or 'p' to get potentiometer reading\r\n", 47 );
    uartUsb.write( "Press 'f' or 'F' to get lm35 reading in Fahrenheit\r\n", 52 );
    uartUsb.write( "Press 'c' or 'C' to get lm35 reading in Celsius\r\n\r\n", 51 );
}

bool areEqual()
{
    int i;

    for (i = 0; i < NUMBER_OF_KEYS; i++) {
        if (codeSequence[i] != buttonsPressed[i]) {
            return false;
        }
    }

    return true;
}

float analogReadingScaledWithTheLM35Formula( float analogReading )
{
    return ( analogReading * 3.3 / 0.01 );
}

float celsiusToFahrenheit( float tempInCelsiusDegrees )
{
    return ( tempInCelsiusDegrees * 9.0 / 5.0 + 32.0 );
}
