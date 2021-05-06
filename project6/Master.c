#include <msp430.h> 

// constants
const char slaveAddrLED = 0x0012;            // LED address
const char slaveAddrLCD = 0x0014;            // LCD address
const char slaveAddrRTC = 0x0068;            // RTC address
const char slaveAddrLM92 = 0x0048;           // LM92 address
const int sendDataLength = 13;               // length of the sendData array
const int refreshSeconds = 2;                // LCD refresh rate in seconds


// global counters
unsigned int readIndexLM19 = 0;             // global variable to populate tempDataLM19 array
unsigned int readIndexLM92 = 0;             // global variable to populate tempDataLM92 array
int timerCount = 0;                         // global timer count to detect 0.5s, 0.33s, and 1s
unsigned int globalIndex;                   // global index to send in I2C ISR


// LCD variables
int isLCDdata = 1;                          // 0 is YES, 1 is NO
unsigned int LCDrefresh = 1;                // LCD refresh flag


// LMxx variables
int LM92data;                               // initial data we get from I2C
unsigned int LM92refresh = 1;               // 0 is YES, 1 is NO
int tempDataLM19[9];                        // array where each ADC value reading from LM19 is stored
int tempDataLM92[9];                        // array where each ADC value reading from LM92 is stored
char sendData[sendDataLength];              // array that we will be sending to the LCD
unsigned int LM92DataCount = 0;
char LM92dataUpper, LM92dataLower;


// LED variables
int patternCount = 1;                       // keep track of LED patterns
char LEDout = 0b01010101;                                // char to send to LED
unsigned int LEDrefresh = 1;                // LED flag


// keypad variables
unsigned int keypadInt = 0;                 // most recent number keypad value (pressed)
char keypadChar = 'D';                      // most recent letter keypad value (pressed)
char sendChar;                              // character sent to RTC (register) or LED (keypad letter)


// RTC variables
char minutes;                               // minutes - encoded in hex but value is int
char seconds;                               // seconds - encoded in hex but value is int
unsigned int rtcDataCount = 0;              // start counter at 0
unsigned int getRTCdata = 0;                // get RTC data flag


// program functions
void getTimeFromRTC();
float movingAverage(unsigned int, int);
void populateSendData(int, float, float, int);
void sendToLCD(char);
int getRTCseconds();
void getTempFromLM92();
void LEDpattern();
void sendToLED();
float LM92conversion(int);


// helper functions
char checkKeypad();
int delay(int);
int _delay1ms();
char _getCharacter(char);
int _toInt(char);
char _toChar(int);
float _adcToTempC(float);
char _hexToChar(char);

/**
 * Still Missing:
 * - LM19 temp correct value
 */


int main(void) {
    WDTCTL = WDTPW | WDTHOLD;           // stop watchdog timer

     // Setup I2C ports
    UCB1CTLW0 |= UCSWRST;               // put into software reset

    UCB1CTLW0 |= UCSSEL_3;              // choose SMCLK
    UCB1BRW = 10;                       // divide by 10 to get SCL = 100kHz

    UCB1CTLW0 |= UCMODE_3;              // put into I2C mode
    UCB1CTLW0 |= UCMST;                 // put into Master mode
    UCB1CTLW0 |= UCTR;                  // put into Tx/WRITE mode

    UCB1CTLW1 |= UCASTP_2;              // AUTO stop when UCB1CNT reached
    UCB1TBCNT = 0x01;                   // send 1 byte of data

    // configure I2C ports
    P4SEL1 &= ~BIT7;                    // set P4.7 as SCL
    P4SEL0 |= BIT7;
    P4SEL1 &= ~BIT6;                    // set P4.6 as SDA
    P4SEL0 |= BIT6;

    //Setup Timer compare
    //divide by 4 and 16 bit with SMClock
    //triggers about 3 times a second
    TB0CTL |= TBCLR;                    //Clear timers and dividers
    TB0CTL |= TBSSEL__ACLK;             // ACLK
    TB0CTL |= MC__UP;                   //count up to value in TB0CCR0
    TB0CTL |= CNTL_1;                   //use 12-bit counter
    TB0CTL |= ID__8;                    //use divide by 8

    TB0CCR0 = 0x2AA;                    // count up to approx 332ms (read values 3 times a second)
    TB0CCTL0 &= ~CCIFG;                 //Clear Flag


    // configure ADC ports
    P5SEL1 |= BIT0;                     // set P5.0 for channel A2
    P5SEL0 |= BIT0;

    UCB1CTLW0 &= ~UCSWRST;               // put out of software reset
    PM5CTL0 &= ~LOCKLPM5;                // Turn on I/O

    // configure ADC
    ADCCTL0 &= ~ADCSHT;                  // clear ADCSHT from def. of ADCSHT=01
    ADCCTL0 |= ADCSHT_2;                 // conversion cycles = 16 (ADCSHT=10)
    ADCCTL0 |= ADCON;                    // turn ADC on

    ADCCTL1 |= ADCSSEL_2;                // ADC clock source = SMCLK
    ADCCTL1 |= ADCSHP;                   // sample signal source = sampling timer
    ADCCTL2 &= ~ADCRES;                  // Clear ADCRES from def. of ADCRES=01
    ADCCTL2 |= ADCRES_2;                 // resolution = 12-bit (ADCRES=10)
    ADCMCTL0 |= ADCINCH_8;               // ADC input channel = A2 (P1.2)

    UCB1IE |= UCTXIE0;                   // Enable I2C TX0 IRQ (transmit reg is ready)
    UCB1IE |= UCRXIE0;                   // Enable I2C RX0 IRQ (receive reg is ready)
    TB0CCTL0 |= CCIE;                    // local enable timer interrupt
    __enable_interrupt();                // global enable

    P5DIR |= BIT1;                       // output P5.2 (Peltier HOT)
    P5DIR |= BIT2;                       // output P5.3 (Peltier COLD)

    P5OUT &= ~BIT1;                      // start P5.2 as OFF (HOT)
    P5OUT &= ~BIT2;                      // start P5.3 as OFF (COLD)

    // variables to populate sendData
    float averageLM19 = -1.0;                         // initialize average variable for LM19
    float averageLM92 = -1.0;                         // initialize average variable for LM92
    float LM19tempC, LM92tempC;


    // testing loop here
    while(1) {

        int currentTimeSeconds = getRTCseconds();

        if(LM92refresh == 0) {
            getTempFromLM92();                                      // acquire temperature from LM92
            averageLM92 = movingAverage(keypadInt, 2);              // calculate the moving average LM92 based on the num pressed
            LM92tempC = LM92conversion(averageLM92);                // convert average LM92 value to temperature
        }

        if(getRTCdata == 1) {                                       // timer reached 1s, start getting RTC data
            getTimeFromRTC();                                       // populate RTC time in global variables `minutes` and `seconds`
        }

        if(LCDrefresh == 2 && keypadInt != 0) {                     // refresh if it's been 2 seconds and keypad 0 is not pressed
            sendToLCD('n');                                         // send `sendData[]` to LCD (no refresh)
        }



        if(keypadInt != 0) {                                        // if an average number was identified, calculate moving average
            while(averageLM19 == -1.0) {
                averageLM19 = movingAverage(keypadInt, 1);                              // calculate the moving average of LM19 based on the num pressed
            }

            LM19tempC = _adcToTempC(averageLM19);                                  // calculate temp in celsius based on ADC value

            populateSendData(keypadInt, LM19tempC, LM92tempC, currentTimeSeconds);      // populate sendData array for I2C transmission
        }


        char keypad = checkKeypad();              // check keypad value. Will return '\0' if nothing

        if(keypad != '\0') {                      // if a button was selected during this loop cycle

            // If a letter was pressed, send signals to the peltier
            if(keypad == 'A' || keypad == 'B' || keypad == 'C' || keypad == 'D') {
                keypadChar = keypad;

                if(keypadChar == 'A') {                 // start heating
                    P5OUT |= BIT1;
                    P5OUT &= ~BIT2;
                }
                else if(keypadChar == 'B') {            // start cooling
                    P5OUT &= ~BIT1;
                    P5OUT |= BIT2;
                }
                else if(keypadChar == 'D') {            // turn off fan
                    P5OUT &= ~BIT1;
                    P5OUT &= ~BIT2;
                }

                sendToLED();
                delay(10);

            }
            else if(keypad == '*' || keypad == '#') {}  // do nothing

            else {                                      // if a number was pressed
                keypadInt = _toInt(keypad);             // update global variable to newest keypad press
            }

//            LM92tempC = 19.5;
//            LM19tempC = 29.5;


            populateSendData(keypadInt, LM19tempC, LM92tempC, currentTimeSeconds);          // populate sendData array for I2C transmission

            sendToLCD('y');                                                                 // send reset character to LCD
            timerCount = 0;                                                                 // reset timer counter
            sendToLCD('n');                                                                 // send currently populated sendData
            delay(5);                                                                       // small delay to pick up keypad presses

        }
    }

    return 0;
}

/**
 * Function that initiates communication to LED
 * LED address is 0x0012
 * Will send the value in `LEDout` (determined in `LEDpattern()`)
 */
void sendToLED() {

    UCB1I2CSA = slaveAddrLED;                               // slave address to 0x12 (LED slave address)
    sendChar = keypadChar;                                      // `LEDout` will be sent to LED
    UCB1CTLW0 |= UCTR;                                      // Put into Tx mode
    UCB1TBCNT = 0x01;                                       // Send 1 byte of data at a time
    UCB1CTLW0 |= UCTXSTT;                                   // Generate START, triggers ISR right after
    while((UCB1IFG & UCSTPIFG) == 0) {}                     // Wait for START, S.Addr, & S.ACK (Reg UCB1IFG contains UCSTPIFG bit)
    UCB1IFG &= ~UCSTPIFG;

}

/**
 * Grabs the minute and second (encoded in hex) and returns the value in seconds
 */
int getRTCseconds() {
    char bottomNibble, topNibble;
    char s = seconds;

    bottomNibble = (s << 4);                                                                // left shift to remove top nibble
    bottomNibble = bottomNibble >> 4;                                                       // right shift to get correct value
    topNibble = 0x00 | s >> 4;                                                              // grab top nibble
    int secondsInt = _toInt(_hexToChar(topNibble)) * 10 + _toInt(_hexToChar(bottomNibble)); // compute seconds

    s = minutes;
    bottomNibble = (s << 4);
    bottomNibble = bottomNibble >> 4;
    topNibble = 0x00 | s >> 4;
    int minutesInt = _toInt(_hexToChar(topNibble)) * 10 + _toInt(_hexToChar(bottomNibble)); // compute minutes


    return minutesInt * 60 + secondsInt;                                                    // return time in seconds
}

/**
 * Computes the value to send to the LED
 * `LEDout` value will be sent to LED
 */
void LEDpattern() {
    int tempVar;                            // temporary variable

    //alternates between 2 led patterns when not cooling or heating
    if(keypadChar == 'D') {

        if(patternCount == 0) {
            LEDout = 0b01010101;
            patternCount = 1;
        }
        else if(patternCount == 1) {
            LEDout = 0b10101010;
            patternCount = 0;
        }
    }

    else if(keypadChar == 'B') {            // pattern for cooling

        if (patternCount == 9) {
            LEDout = 0;
            patternCount = 0;
        }
        else if(patternCount == 1) {
            LEDout = 0b10000000;
        }
        else {
            tempVar = LEDout >> 1;          // rotates the bits and adds one to the end so scrolling 1's MSB to LSB
            LEDout = tempVar | LEDout;
        }

        patternCount++;
    }

    else if(keypadChar == 'A') {            // pattern for heating

        if (patternCount == 9) {
            LEDout = 0;
            patternCount = 0;
        }
        else if(patternCount == 1) {
            LEDout = 0b00000001;
        }
        else {
            tempVar = LEDout << 1;         // same as B but from LSB to MSB
            LEDout = tempVar | LEDout;
        }

        patternCount++;
    }
}

/**
 * Converts values to char
 * Populates sendData array with chars
 * Will be sending: keypadInt, keypadChar, RTC number of seconds, tempLM19, tempLM92
 * Example array: [ '5', '13.4',  'A', '290', '29.1']
 * -> ['5', '1', '3', '.', '4', 'A', '2', '9', '0', '2', '9', '.', '1']
 */
void populateSendData(int kInt, float tempLM19, float tempLM92, int RTCseconds) {
    int index;

    // position 0
    sendData[0] = _toChar(kInt);

    // from position 1 -> 4
    float fullValueFloat = tempLM19 * 10;
    int fullValue = (int) fullValueFloat;                               // since a decimal number, make it a full number: 13.5 -> 135
    for(index = 4; index > 0; index--) {
        if(index == 3) {                                        // if third iteration, add decimal point '.'
            sendData[index] = '.';
            continue;
        }

        int integerValue = (int) fullValue % 10;                // mod `fullValue` and cast to integer 135 % 10 = (int) 5
        char charValue = _toChar(integerValue);                 // convert new `integerValue` to char 5 -> '5'
        sendData[index] = charValue;                            // add charValue into sendData array
        fullValue /= 10;                                        // divide down number
    }

    // position 5
    sendData[5] = keypadChar;

    // from position 6 -> 8
    for(index = 8; index > 5; index--) {

        int integerValue = (int) RTCseconds % 10;                // mod `fullValue` and cast to integer 135 % 10 = (int) 5
        char charValue = _toChar(integerValue);                 // convert new `integerValue` to char 5 -> '5'
        sendData[index] = charValue;                            // add charValue into sendData array
        RTCseconds /= 10;                                        // divide down number
    }

    // from position 9 -> 12
    fullValueFloat = tempLM92 * 10;
    fullValue = (int) fullValueFloat;
    for(index = 12; index > 8; index--) {
        if(index == 11) {                                        // if third iteration, add decimal point '.'
            sendData[index] = '.';
            continue;
        }

        int integerValue = (int) fullValue % 10;                // mod `fullValue` and cast to integer 135 % 10 = (int) 5
        char charValue = _toChar(integerValue);                 // convert new `integerValue` to char 5 -> '5'
        sendData[index] = charValue;                            // add charValue into sendData array
        fullValue /= 10;                                        // divide down number
    }
}


/**
 * Generates start condition to LCD device
 * Send contents in sendData[]
 * refresh = 'y' -> send refresh bit to LCD
 */
void sendToLCD(char refresh) {

    if(refresh == 'y') {
        UCB1I2CSA = slaveAddrLCD;                               // slave address to 0x14 (LCD slave address)
        sendChar = '*';                                         // '*' will be sent to LCD
        UCB1CTLW0 |= UCTR;                                      // Put into Tx mode
        UCB1TBCNT = 0x01;                                       // Send 1 byte of data at a time
        UCB1CTLW0 |= UCTXSTT;                                   // Generate START, triggers ISR right after
        while((UCB1IFG & UCSTPIFG) == 0) {}                     // Wait for START, S.Addr, & S.ACK (Reg UCB1IFG contains UCSTPIFG bit)
        UCB1IFG &= ~UCSTPIFG;
        return;
    }

    isLCDdata = 0;                                              // enable LCD data transmission

    for(globalIndex = 0; globalIndex < sendDataLength; globalIndex++) {
        UCB1I2CSA = slaveAddrLCD;                               // slave address to 0x14 (LCD slave address)
        UCB1CTLW0 |= UCTR;                                      // Put into Tx mode
        UCB1TBCNT = 0x01;                                       // Send 1 byte of data at a time
        UCB1CTLW0 |= UCTXSTT;                                   // Generate START, triggers ISR right after


        while((UCB1IFG & UCSTPIFG) == 0) {}                     // Wait for START, S.Addr, & S.ACK (Reg UCB1IFG contains UCSTPIFG bit)
        UCB1IFG &= ~UCSTPIFG;                                   // Clear flag
    }

    isLCDdata = 1;                                              // disable LCD data transmission flag
}

//converts LM92 raw data to Celsius
float LM92conversion(int LM92dataFromI2C){
    float final;         //initialize a float
    LM92dataFromI2C = LM92dataFromI2C & ~(BIT0 | BIT1 | BIT2); //clear off the status bits
    LM92dataFromI2C = LM92dataFromI2C >> 3;            //move the binary number to the LSB
    final = LM92dataFromI2C * .0625;            //multiply by .0625 to get the celsius number
    return final;
}

/**
 * Get's current temp value from LM92
 */
void getTempFromLM92() {

    UCB1I2CSA = slaveAddrLM92;                 // Set Slave Address to LM92 -> 0x0048
    UCB1CTLW0 &= ~UCTR;                        // Put into Rx mode
    UCB1TBCNT = 0x02;                          // Send 1 byte of data at a time
    UCB1CTLW0 |= UCTXSTT;                      // Generate START

    while((UCB1IFG & UCSTPIFG) == 0) {}        // wait for stop condition
    UCB1IFG &= ~UCSTPIFG;                      // Clear flag

    LM92data = (LM92dataUpper << 8) | LM92dataLower;

    if(readIndexLM92 == keypadInt) {                // reset index if limit is exceeded
        readIndexLM92 = 0;
    }

    tempDataLM92[readIndexLM92] = LM92data;         // put the data received from I2C into array

    readIndexLM92++;                                // increment readIndex for next position

    LM92refresh = 1;                                // turn off LM92 data fetching
}

/**
 * Function that gets time data from RTC
 * This will only occur every second.
 * It is dependent from the `getRTCdata` flag.
 * Flag gets toggled in `ISR_TB0_CCR0()` ISR
 */
void getTimeFromRTC() {

     // Request time in [ss/mm] format
     UCB1I2CSA = slaveAddrRTC;                  // Set Slave Address
     UCB1CTLW0 |= UCTR;                         // Put into Tx mode
     UCB1TBCNT = 0x01;                          // Send 1 byte of data at a time
     sendChar = 0x00;                           // send RTC register to start the read from
     UCB1CTLW0 |= UCTXSTT;                      // Generate START

     while((UCB1IFG & UCSTPIFG) == 0) {}        // Wait for START, S.Addr, & S.ACK (Reg UCB1IFG contains UCSTPIFG bit)
     UCB1IFG &= ~UCSTPIFG;                      // Clear flag


     // Receive Requested Data from RTC
     UCB1I2CSA = slaveAddrRTC;                  // Set Slave Address
     UCB1CTLW0 &= ~UCTR;                        // Put into Rx mode
     UCB1TBCNT = 0x02;                          // Receive 2 bytes of data at a time
     UCB1CTLW0 |= UCTXSTT;                      // Generate START

     while((UCB1IFG & UCSTPIFG) == 0) {}
     UCB1IFG &= ~UCSTPIFG;                      // Clear flag

     getRTCdata = 0;                            // stop getting data from RTC
}


/**
 * Calculates the moving average and populates either tempDataLM19 or tempDataLM92
 * @params `averageNum` refers to the number of the moving average
 * @params `sensor` depicts which array to populate: either '19' or '92'
 * GOAL: populate the tempDataLMxx array
 *
 * Steps:
 *  tempDataLMxx[9] -> upon initialization, they all start with 0s.
 *  tempDataLMxx is populated with the timer interrupt.
 *  every second it will populate the tempDataLMxx array within
 */
float movingAverage(unsigned int averageNum, int sensor) {
//    a = averageNum;

    // if tempData is not fully populated yet, return back to main
    // prevent index out of bounds error
    if(averageNum == 0) return 0.0;

    if(sensor == 1) {
        if(tempDataLM19[averageNum - 1] == 0) return -1.0;
    }
    else if(sensor == 2) {
        if(tempDataLM92[averageNum - 1] == 0) return -1.0;
    }

    int index;
    float average = 0;

    if(sensor == 1) {
        // if tempDataLM19 is populated, add up all the values within array
        for(index = 0; index < averageNum; index++) {
            average += tempDataLM19[averageNum - 1];
        }
    }
    else if(sensor == 2) {
        // if tempDataLM92 is populated, add up all the values within array
        for(index = 0; index < averageNum; index++) {
            average += tempDataLM92[averageNum - 1];
        }
    }

    return average / averageNum;
}

/**
 * Helper functions:
 */

// Converts char to int
int _toInt(char c) {
    return c - '0';
}

// Converts int to char
char _toChar(int i) {
    return i + '0';
}

// Converts celcius to Kelvin
int _toKelvin(float tempC) {
    return tempC + 273;

}

float _adcToTempC(float averageADCvalue) {
//    return (-0.284 * averageADCvalue) + 289.89 + 4;
//    return -0.0704 * averageADCvalue + 158.11;
    return -0.0704 * averageADCvalue + 287.89;
}

char _hexToChar(char hex) {
    switch(hex) {
        case 0x01:
            return '1';
        case 0x02:
            return '2';
        case 0x03:
            return '3';
        case 0x04:
            return '4';
        case 0x05:
            return '5';
        case 0x06:
            return '6';
        case 0x07:
            return '7';
        case 0x08:
            return '8';
        case 0x09:
            return '9';
        default:
            return '0';
    }
}


// checks for the button pressed on keypad
char checkKeypad() {

    // configure inputs
    P1DIR &= ~BIT0;              // Configure P1.0 (LED1) as input
    P1DIR &= ~BIT1;              // Configure P1.1 (LED1) as input
    P1DIR &= ~BIT2;              // Configure P1.2 (LED1) as input
    P1DIR &= ~BIT3;              // Configure P1.3 (LED1) as input

    // set inputs as 0
    P1OUT &= ~BIT0;              // Configure P1.0 (LED1) as input
    P1OUT &= ~BIT1;              // Configure P1.1 (LED1) as input
    P1OUT &= ~BIT2;              // Configure P1.2 (LED1) as input
    P1OUT &= ~BIT3;              // Configure P1.3 (LED1) as input

    //configure outputs
    P1DIR |= BIT4;              // Configure P1.4 (LED1) as output
    P1DIR |= BIT5;              // Configure P1.5 (LED1) as output
    P1DIR |= BIT6;              // Configure P1.6 (LED1) as output
    P1DIR |= BIT7;              // Configure P1.7 (LED1) as output

    // turn on outputs
    P1OUT |= BIT4;              // Configure P1.4 (LED1) as output
    P1OUT |= BIT5;              // Configure P1.5 (LED1) as output
    P1OUT |= BIT6;              // Configure P1.6 (LED1) as output
    P1OUT |= BIT7;              // Configure P1.7 (LED1) as output

    // setup pull-up/down resistors
    P1REN |= BIT0;              // enable pull up/down resistor on P1.0
    P1REN |= BIT1;              // enable pull up/down resistor on P1.1
    P1REN |= BIT2;              // enable pull up/down resistor on P1.2
    P1REN |= BIT3;              // enable pull up/down resistor on P1.3

    // configure all resistors as pull up
    P1OUT &= ~BIT0;
    P1OUT &= ~BIT1;
    P1OUT &= ~BIT2;             // P1.2
    P1OUT &= ~BIT3;             // P1.3

    char buttonPressed = 0x00;      // initialize as 0

    char currentSnapshot = P1IN;

    // first switch statement
    switch(currentSnapshot) {
        case 0xF8:
            buttonPressed = 0x08;
            break;

        case 0xF4:
            buttonPressed = 0x04;
            break;

        case 0xF2:
            buttonPressed = 0x02;
            break;

        case 0xF1:
            buttonPressed = 0x01;
            break;

        default:
            return '\0';
        }

    //configure inputs
    P1DIR &= ~BIT4;              // Configure P1.4 as output
    P1DIR &= ~BIT5;              // Configure P1.5 as output
    P1DIR &= ~BIT6;              // Configure P1.6 as output
    P1DIR &= ~BIT7;              // Configure P1.7 as output


    //configure outputs
    P1DIR |= BIT0;              // Configure P1.0 as output
    P1DIR |= BIT1;              // Configure P1.1 as output
    P1DIR |= BIT2;              // Configure P1.2 as output
    P1DIR |= BIT3;              // Configure P1.3 as output

    // turn on outputs
    P1OUT |= BIT0;              // Configure P1.0 as output
    P1OUT |= BIT1;              // Configure P1.1 as output
    P1OUT |= BIT2;              // Configure P1.2 as output
    P1OUT |= BIT3;              // Configure P1.3 as output

    // setup pull-up/down resistors
    P1REN |= BIT4;              // enable pull up/down resistor on P1.4
    P1REN |= BIT5;              // enable pull up/down resistor on P1.5
    P1REN |= BIT6;              // enable pull up/down resistor on P1.6
    P1REN |= BIT7;              // enable pull up/down resistor on P1.7

    // configure all resistors as pull up
    P1OUT &= ~BIT4;
    P1OUT &= ~BIT5;
    P1OUT &= ~BIT6;
    P1OUT &= ~BIT7;

    currentSnapshot = P1IN;

    // second switch statement
    switch(currentSnapshot) {
        case 0x8F:
            buttonPressed ^= 0x80;
            break;

        case 0x4F:
            buttonPressed ^= 0x40;
            break;

        case 0x2F:
            buttonPressed ^= 0x20;
            break;

        case 0x1F:
            buttonPressed ^= 0x10;
            break;

        default:
            return '\0';
    }

    return _getCharacter(buttonPressed);
}

// translates the button pressed to ASCII
char _getCharacter(char buttonPressed) {
    char button;

    switch(buttonPressed) {
        case 0x88:
            button = '1';
            break;
        case 0x84:
            button = '2';
            break;
        case 0x82:
            button = '3';
            break;
        case 0x81:
            button = 'A';
            break;

        case 0x48:
            button = '4';
            break;
        case 0x44:
            button = '5';
            break;
        case 0x42:
            button = '6';
            break;
        case 0x41:
            button = 'B';
            break;

        case 0x28:
            button = '7';
            break;
        case 0x24:
            button = '8';
            break;
        case 0x22:
            button = '9';
            break;
        case 0x21:
            button = 'C';
            break;

        case 0x18:
            button = '*';
            break;
        case 0x14:
            button = '0';
            break;
        case 0x12:
            button = '#';
            break;
        case 0x11:
            button = 'D';
            break;
        default:
            button = '\0';
            break;
    }
    return button;
}

/**
 * delay: Delay set for approx 1 ms
 */
int delay(int LongCount) {
    while(LongCount > 0) {
        LongCount--;
        _delay1ms();
    }
    return 0;
}

/**
 * Delay1ms: Delay set for approx 1 ms
 */
int _delay1ms() {
    int ShortCount;
    for(ShortCount = 0; ShortCount < 102; ShortCount++) {}
    return 0;
}


// ISRs -------------------------------------------------------------
/**
 * I2C Interrupt Service Routine
 */
#pragma vector=EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void) {

    switch(UCB1IV) {
        case 0x16:                                              // ID16 = RXIFG0 -> Receive
            if(LM92refresh == 0) {

                if(LM92DataCount == 0) {
                    LM92dataUpper = UCB1RXBUF;
                    LM92DataCount++;
                }
                else if(LM92DataCount == 1) {
                    LM92dataLower = UCB1RXBUF;
                    LM92DataCount = 0;
                }
            }

            else if(rtcDataCount == 0) {                        // getting seconds
                seconds = UCB1RXBUF;
                rtcDataCount++;
            }
            else if(rtcDataCount == 1) {
                minutes = UCB1RXBUF;                            // getting minutes
                rtcDataCount = 0;
            }
            break;

        case 0x18:                                              // ID18 = TXIFG0 -> Transmit
            if(isLCDdata == 0) {
                UCB1TXBUF = sendData[globalIndex];              // send current sendData char to LCD
            }
            else {
                UCB1TXBUF = sendChar;                           // send RTC register value + LED value
            }


            break;

        default:
            break;
      }

}

/**
 * Timer Interrupt Service Routine
 * Takes in ADC value from LM-19 and populates tempData array
 * Fires every 333 ms
 */
#pragma vector=TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void) {

    if(timerCount == 3 || timerCount == 6) {        // every 0.5s, grab temp values
        ADCCTL0 |= ADCENC | ADCSC;                  // grab LM19 Temp data
        while((ADCIFG & ADCIFG0) == 0);             // wait to receive all data

        if(keypadInt == 0) {
            // don't populate / do nothing
        }

        else if(readIndexLM19 == keypadInt) {           // once the amount of desired data points is obtained
            readIndexLM19 = 0;                          // reset index
            tempDataLM19[readIndexLM19] = ADCMEM0;      // start overriding values in tempDataLM19
        }
        else {
            tempDataLM19[readIndexLM19] = ADCMEM0;      // populate tempDataLM19
        }

        readIndexLM19++;


        LM92refresh = 0;                            // enable flag to get LM92 data in main subroutine
    }


    if(timerCount == 6) {                           // if 1.0s, get time data from rtc
        timerCount = -1;                             // reset timerCount
        getRTCdata = 1;                             // start getting RTC data in `main()`

        if(LCDrefresh == refreshSeconds) {
            LCDrefresh = 1;                         // restart 2 second count
        }
        else {
            LCDrefresh++;                           // count up seconds to refresh LCD
        }

    }

    timerCount++;                                   // increment global timer variable

    TB0CCTL0 &= ~CCIFG;                             // Clear Flag
}

