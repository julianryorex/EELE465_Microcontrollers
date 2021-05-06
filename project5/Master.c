#include <msp430.h> 

// constants
const char slaveAddrLED = 0x0012;
const char slaveAddrLCD = 0x0014;
const int sendDataLength = 8;               // length of the sendData array

// index variables
unsigned int globalIndex;                   // global index to send in I2C ISR
int readIndex = 0;                          // index used to read tempData in ISR

// Flags
int isLCDdata = 1;                          // 0 is YES, 1 is NO

// global variables
char keypad;                                // character pressed from keypad
int tempData[9];                            // array where each ADC value reading is stored.
char sendData[sendDataLength];              // array of that we will be sending to the LCD

// test data
float tempC;
int tempK;


// main program functions
float movingAverage(unsigned int);
void populateSendData(float, int);
void sendToLCD();
char checkKeypad();
int delay(int);
void resetTempData();

// helper functions
char _getCharacter(char);
int _toInt(char);
char _toChar(int);
int _delay1ms();
int _toKelvin(float);
float _adcToTempC(float);

/**
 * TODO:
 * - test with LED bar
 * - tweak offset in `_adcToTempC()`
 */


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // stop watchdog timer

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

        TB0CCR0 = 0x555;                    // count up to approx 332ms (read values 3 times a second)
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
       ADCCTL2 |= ADCRES_1;                 // resolution = 12-bit (ADCRES=10)
       ADCMCTL0 |= ADCINCH_8;               // ADC input channel = A2 (P1.2)

       UCB1IE |= UCTXIE0;                   // Enable I2C TX0 IRQ (transmit reg is ready)
       TB0CCTL0 |= CCIE;                    // local enable timer interrupt
       __enable_interrupt();                // global enable


       while(1) {
          keypad = checkKeypad();                               // check keypad value. Will return '\0' if nothing

          if(keypad != '\0') {

              // if button pressed is *, send it to LCD to reset.
              if(keypad == '*') {
                  resetTempData();
                  UCB1I2CSA = slaveAddrLCD;                     // slave address to 0x14 (LCD slave address)
                  UCB1CTLW0 |= UCTXSTT;                         //Generate START, triggers ISR right after
                  delay(200);
              }

              // if keypad corresponds to an LED pattern
              else if(keypad == 'A' || keypad == 'B' || keypad == 'C' || keypad == 'D') {
                  UCB1I2CSA = slaveAddrLED;                     // slave address to 0x12 (LED slave address)
                  UCB1CTLW0 |= UCTXSTT;                         //Generate START, triggers ISR right after
                  delay(200);
              }

              // if a number is pressed
              else {
                  int keypadNum = _toInt(keypad);               // get the integer value of the number pressed
                  if(keypadNum == 0) continue;

                  float average = -1.0;

                  while(average == -1.0) {
                      average = movingAverage(keypadNum);       // calculate the moving average based on the num pressed
                  }

                  tempC = _adcToTempC(average);       // calculate temp in celcius based on ADC value
                  tempK = _toKelvin(tempC / 10);             // convert temp in C to Kelvin
                  populateSendData(tempC / 10, tempK);           // populate sendData array for I2C transmission
                  sendToLCD();                              // iteratively send data to LCD

              }
          }
       }

    return 0;
}

/**
 * Resets global index and tempData[]
 */
void resetTempData() {
    readIndex = 0;                                              // reset global reading index
    globalIndex = 0;

    tempData[0] = 0;                                            // reset tempData array
    tempData[1] = 0;
    tempData[2] = 0;
    tempData[3] = 0;
    tempData[4] = 0;
    tempData[5] = 0;
    tempData[6] = 0;
    tempData[7] = 0;
    tempData[8] = 0;

    sendData[0] = 0;                                            // reset tempData array
    sendData[1] = 0;
    sendData[2] = 0;
    sendData[3] = 0;
    sendData[4] = 0;
    sendData[5] = 0;
    sendData[6] = 0;
    sendData[7] = 0;
    sendData[8] = 0;
    sendData[9] = 0;
}

/**
 * Generates start condition to LCD device
 * Send contents in sendData[]
 */
void sendToLCD() {
    isLCDdata = 0;                                              // enable LCD data transmission

    for(globalIndex = 0; globalIndex < sendDataLength; globalIndex++) {
        UCB1I2CSA = slaveAddrLCD;                               // slave address to 0x14 (CD slave address)
        UCB1CTLW0 |= UCTXSTT;                                   //Generate START, triggers ISR right after
        delay(300);
    }

    isLCDdata = 1;
}

/**
 * Converts values to char
 * Populates sendData array with chars
 * Example array: ['2', '9', '8', '1', '3', '.' '5', '\0'] --> 298 Kelvin, 13.5 C
 */
void populateSendData(float tempC, int tempK) {
    int index;

    float fullValue = tempC * 10;                               // since a decimal number, make it a full number: 13.5 -> 135
    for(index = sendDataLength - 1; index > 2; index--) {
        if(index == sendDataLength - 1) {                       // if first iteration, add null character
            sendData[index] = '\0';
            continue;
        }
        if(index == 5) {                                        // if third iteration, add decimal point '.'
            sendData[index] = '.';
            continue;
        }

        int integerValue = (int) fullValue % 10;                // mod `fullValue` and cast to integer 135 % 10 = (int) 5
        char charValue = _toChar(integerValue);                 // convert new `integerValue` to char 5 -> '5'
        sendData[index] = charValue;                            // add charValue into sendData array
        fullValue /= 10;                                        // divide down number
    }

    // same process but with the kelvin value. No need to convert to full value because tempK is type [int]
    for(index = 2; index > -1; index--) {
        int integerValue = tempK % 10;
        char charValue = _toChar(integerValue);
        sendData[index] = charValue;
        tempK /= 10;
    }
}



/**
 * Calculates the moving average and populates the global tempData array
 * @params `averageNum` refers to the number of the moving average
 * GOAL: populate the tempData array
 *
 * Steps:
 *  tempData[9] -> upon initialization, they all start with 0s.
 *  tempData is populated with the timer interrupt.
 *  every second it will populate the tempData array within
 */
float movingAverage(unsigned int averageNum) {

    // if tempData is not fully populated yet, return back to main
    // prevent index out of bounds error
    if(averageNum == 0) return 0.0;
    if(tempData[averageNum - 1] == 0) return -1.0;

    int index;
    float average = 0;

    // if tempData is populated, add up all the values within array
    for(index = 0; index < averageNum; index++) {
        average += tempData[averageNum - 1];
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
    return (-0.284 * averageADCvalue) + 289.89 + 4;
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
} //-----------------End Delay1ms ------------------------------------


/**
 * Delay1ms: Delay set for approx 1 ms
 */
int _delay1ms() {
    int ShortCount;
    for(ShortCount = 0; ShortCount < 102; ShortCount++) {}
    return 0;
}




// ISRs -------------------------------------------------------------
#pragma vector=EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void) {
    if(isLCDdata == 0) {
        UCB1TXBUF = sendData[globalIndex];          // send current sendData char to LCD
    }
    else {
        UCB1TXBUF = keypad;                         // send keypad value
    }
}

/**
 * Timer Interrupt Service Routine
 * Takes in ADC value from LM-19 and populates tempData array
 * Fires every 333 ms
 */
#pragma vector=TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void) {
    ADCCTL0 |= ADCENC | ADCSC;
    while((ADCIFG & ADCIFG0) == 0);

    if(readIndex == _toInt(keypad)) {
        readIndex = 0;
        tempData[readIndex] = ADCMEM0;
        readIndex++;
    }
    else {
        tempData[readIndex] = ADCMEM0;
        readIndex++;
    }

    TB0CCTL0 &= ~CCIFG;                             //Clear Flag
}

