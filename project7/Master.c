#include <msp430.h> 

// constants
const char slaveAddrLED = 0x0012;               // LED address
const char slaveAddrLCD = 0x0014;               // LCD address
const char slaveAddrRTC = 0x0068;               // RTC address
const unsigned int lightThreshold = 130;        // ADC value threshold to distinguish if light or dark
const unsigned int ADCcount = 4;                // number of ADCs in the circuit
const unsigned int sendDataLength = 5;          // length of sendData array

// global variables
int tempData[ADCcount];                         // array where ADC values are stored
char sendData[sendDataLength];                  // array that will be sent to LCD
unsigned int player1Score = 2, player2Score = 2;// players' scores
char sendSpecialChar;                           // special character that will be sent to LCD/LED
unsigned int globalIndex = 0;                   // global index used to send data via I2C

// LCD variables
int isLCDdata = 1;                          // 0 is YES, 1 is NO


// RTC variables
char minutes;                               // minutes - encoded in hex but value is int
char seconds;                               // seconds - encoded in hex but value is int
unsigned int rtcDataCount = 0;              // start counter at 0
unsigned int getRTCdata = 1;                // get RTC data flag
unsigned int lastMinutes = 0;


// functions
void getADCvalues();
void calculatePlayerScores();
void populateSendData();
void getTimeFromRTC();
int getRTCseconds();
char getRTCminutes(char);
void sendToLCD(char);
void sendToLED(char);


// helper functions
char checkKeypad();
int delay(int);
int _delay1ms();
char _getCharacter(char);
int _toInt(char);
char _toChar(int);
char _hexToChar(char);
int _adcConversion(unsigned int);
char _hexToChar(char);


/**
 * main.c
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
    TB0CTL |= TBCLR;                    // Clear timers and dividers
    TB0CTL |= TBSSEL__ACLK;             // ACLK
    TB0CTL |= MC__UP;                   // count up to value in TB0CCR0
    TB0CTL |= CNTL_1;                   // use 12-bit counter
    TB0CTL |= ID__8;                    // use divide by 8

    TB0CCR0 = 0x2AA;                    // count up to approx 166ms (read values 3 times a second)
    TB0CCTL0 &= ~CCIFG;                 // Clear Flag


    // configure ADC ports
    P5SEL1 |= BIT0;                      // set P5.0 for channel A8 (P1)
    P5SEL0 |= BIT0;

    P5SEL1 |= BIT1;                      // set P5.1 for channel A9 (P1)
    P5SEL0 |= BIT1;

    P5SEL1 |= BIT2;                      // set P5.2 for channel A10 (P2)
    P5SEL0 |= BIT2;

    P5SEL1 |= BIT3;                      // set P5.3 for channel A11 (P2)
    P5SEL0 |= BIT3;

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
    ADCMCTL0 |= ADCINCH_8;               // ADC input channel = A8 (P5.0)

    UCB1IE |= UCTXIE0;                   // Enable I2C TX0 IRQ (transmit reg is ready)
    UCB1IE |= UCRXIE0;                   // Enable I2C RX0 IRQ (receive reg is ready)
//    TB0CCTL0 |= CCIE;                    // local enable timer interrupt
    __enable_interrupt();                // global enable

    P4DIR |= BIT5;                      // Set P4.5 as output
    P4OUT |= BIT5;                     // start ON

    sendToLCD('y');                     // reset LCD

    unsigned int prevPlayer1Score, prevPlayer2Score;
    unsigned int firstIteration = 0;


    while(1) {

        getADCvalues();                 // populates global array tempData with ADC values
        calculatePlayerScores();        // calculates players scores with tempData

        if(player1Score == 0 || player2Score == 0) {
            getTimeFromRTC();                                                       // get time from RTC via I2C
            populateSendData();
            sendToLCD('n');
            sendToLED('n');

            char keypad;
            do {
                 keypad = checkKeypad();
            } while(keypad != '*');
            player1Score = 2;
            player2Score = 2;


            sendToLCD('y');                             // reset LCD
            sendToLED('y');                             // reset LED

            continue;
        }

        getTimeFromRTC();                                                       // get time from RTC via I2C
        populateSendData();                                                     // populate sendData global array
        sendToLCD('n');                                                         // send sendData to LCD
        sendToLED('n');

        delay(3000);

    }

    return 0;
}




/**
 * Function that initiates communication to LED
 * LED address is 0x0012
 * Will send the value in `LEDout` (determined in `LEDpattern()`)
 */
void sendToLED(char reset) {

    if(reset == 'y') {
        UCB1I2CSA = slaveAddrLED;                               // slave address to 0x14 (LCD slave address)
        sendSpecialChar = '*';                                         // '*' will be sent to LCD
        UCB1CTLW0 |= UCTR;                                      // Put into Tx mode
        UCB1TBCNT = 0x01;                                       // Send 1 byte of data at a time
        UCB1CTLW0 |= UCTXSTT;                                   // Generate START, triggers ISR right after
        while((UCB1IFG & UCSTPIFG) == 0) {}                     // Wait for START, S.Addr, & S.ACK (Reg UCB1IFG contains UCSTPIFG bit)
        UCB1IFG &= ~UCSTPIFG;
        return;
    }

    unsigned int i;

    for(i = 2; i < 4; i++) {
        UCB1I2CSA = slaveAddrLED;                               // slave address to 0x12 (LED slave address)
        sendSpecialChar = sendData[i];                                 //
        UCB1CTLW0 |= UCTR;                                      // Put into Tx mode
        UCB1TBCNT = 0x01;                                       // Send 1 byte of data at a time
        UCB1CTLW0 |= UCTXSTT;                                   // Generate START, triggers ISR right after
        while((UCB1IFG & UCSTPIFG) == 0) {}                     // Wait for START, S.Addr, & S.ACK (Reg UCB1IFG contains UCSTPIFG bit)
        UCB1IFG &= ~UCSTPIFG;
    }

    i = 0;
    char sp = sendSpecialChar;

}


/**
 * Populates global sendData array based on current values
 * sendData is what will be sent to LCD (some to LED)
 * `specialChar is defaulted to 1 == FALSE /|\ 0 == TRUE
 */
void populateSendData() {
    char bottomNibble, topNibble;
    char s = minutes;

    bottomNibble = (s << 4);
    bottomNibble = bottomNibble >> 4;
    topNibble = 0x00 | s >> 4;

    sendData[0] = getRTCminutes(topNibble);
    sendData[1] = getRTCminutes(bottomNibble);

    sendData[2] = _toChar(player1Score);
    sendData[3] = _toChar(player2Score);

    if(player1Score == 0) {                         // if player 1 has score 0, player 2 wins
        sendData[4] = ']';
    }

    else if(player2Score == 0) {                    // if player 2 has score 0, player 1 wins
        sendData[4] = '[';
    }
    else {
        sendData[4] = '!';
    }

}

/**
 * Calculates scores for player 1 and player 2 based on ADC values
 */
void calculatePlayerScores() {
    int i;
    player1Score = 0;                           // reset score to count new score
    player2Score = 0;                           // reset score to count new score

    for(i = 0; i < ADCcount; i++) {
        if(tempData[i] < lightThreshold) {
            if(i < 2) {                         // player 1 score
                player1Score++;
            }
            else {                              // player 2 score
                player2Score++;
            }
        }
    }
}

/**
 * Generates start condition to LCD device
 * Send contents in sendData[]
 * refresh = 'y' -> send refresh bit to LCD
 */
void sendToLCD(char reset) {

    if(reset == 'y') {
        UCB1I2CSA = slaveAddrLCD;                               // slave address to 0x14 (LCD slave address)
        sendSpecialChar = '*';                                         // '*' will be sent to LCD
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
     sendSpecialChar = 0x00;                    // send RTC register to start the read from
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
 * Converts the nibble to a character
 */
char getRTCminutes(char nibble) {
    return _hexToChar(nibble);
}


/**
 * Populates the global array tempData with ADC value
 */
void getADCvalues() {
    unsigned int i;
    for(i = 0; i < ADCcount; i++) {
        tempData[i] = _adcConversion(i);
    }

}

/**
 * Function that changes the ADC channel and retrieves the ADC value
 * returns the ADC value
 */
int _adcConversion(unsigned int index) {
    char adcChannel;

    switch(index) {
        case 0:
            adcChannel = ADCINCH_8;
            break;
        case 1:
            adcChannel = ADCINCH_9;
            break;
        case 2:
            adcChannel = ADCINCH_10;
            break;
        case 3:
        default:
            adcChannel = ADCINCH_11;
            break;
    }


    ADCCTL0 &= ~(ADCENC | ADCSC);
    ADCMCTL0 = adcChannel;

    ADCCTL0 |= ADCENC | ADCSC;                  // grab photocell ADC value
    while((ADCIFG & ADCIFG0) == 0);             // wait to receive all data

    return ADCMEM0;

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

// Converts hex encoded value to Char
// Ex: '0x08' -> '8'
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

// Converts char to int
int _toInt(char c) {
    return c - '0';
}

// Converts int to char
char _toChar(int i) {
    return i + '0';
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
            if(rtcDataCount == 0) {                             // getting seconds
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
                UCB1TXBUF = sendSpecialChar;                           // send LED value
            }

            break;

        default:
            break;
      }

}

