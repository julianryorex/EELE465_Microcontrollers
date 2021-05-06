#include <msp430.h> 

// variables
int Data = 'D';

// LED variables
int patternCount = 1;                       // keep track of LED patterns
char LEDout = 0b01010101;                   // char to send to LED

// implicit declaration of functions
void _displayNumber(int number);
void LEDpattern();
int delay(int);
int _delay1ms();

/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    UCB0CTLW0 |= UCSWRST; //RESET

    UCB0CTLW0 |= UCMODE_3; //SELECT I2C
    UCB0CTLW0 &= ~UCMST;   //slave MODE
    UCB0I2COA0 = 0x0012;     //SLAVE ADDRESS
    UCB0I2COA0 |= UCOAEN;   // slave address enable
    UCB0CTLW0 &= ~UCTR;      //RECEIVE MODE
    UCB0CTLW1 &= ~UCSWACK;  // auto acknowledge
    UCB0TBCNT = 0x01;      // Length of Receiving data


    UCB0CTLW1 &= ~UCTXIFG0;
    P1SEL1 &= ~BIT3;    //P1.3 = SCL
    P1SEL0 |= BIT3;

    P1SEL1 &= ~BIT2;     //P1.2 = SDA
    P1SEL0 |= BIT2;

// Ports 1.0-1.1, 1.4-1.7 are Bits 5-0
    P1DIR |= BIT7;
    P1DIR |= BIT6;
    P1DIR |= BIT5;
    P1DIR |= BIT4;
    P1DIR |= BIT1;
    P1DIR |= BIT0;

    // initialize all LED P1 ports as OFF
    P1OUT &= ~BIT7;
    P1OUT &= ~BIT6;
    P1OUT &= ~BIT5;
    P1OUT &= ~BIT4;
    P1OUT &= ~BIT1;
    P1OUT &= ~BIT0;

//P2.7-2.6 are BITS 7-6
    P2DIR |= BIT7;
    P2DIR |= BIT6;

    P2OUT &= ~BIT7;
    P2OUT &= ~BIT6;

    PM5CTL0 &= ~LOCKLPM5;               // TURN ON DIGITAL I/O

    UCB0CTLW0 &= ~UCSWRST;              // OUT OF RESET

    UCB0IE |= UCRXIE0;                  //ENABLE I2C RX IRQ

    __enable_interrupt();

    while(1){
        LEDpattern();
        _displayNumber(LEDout);
        delay(333);
//        if(Data != 0) {
//
//            Data = 0;
//        }
    }

    return 0;
}

// -------------------------------------------------------------------------------------------------


/**
 * Ports used for counter:
 * P1DIR |= BIT7;
 * P1.0, P1.1, P1.4, P1.5, P1.6, P1.7, P2.6, P2.7
 */
void _displayNumber(int number) {
    // MSB 7     6     5     4     3     2     1     0     LSB
    // MSB P1.7  P1.6  P1.5  P1.4  P2.7  P2.6  P1.1  P1.0  LSB
    char temp;

    P2OUT &= ~(BIT6 | BIT7);            // clear everything in P2 except P2.6 and P2.7
    temp = number;                    // same thing with port 2, temp = counterB
    temp &= (BIT2 | BIT3);              // clear all except PX.2 and PX.3 (temp)
    temp = temp << 4;                   // shift left 4
    P2OUT |= temp;                      // output temp to P2

    P1OUT &= (BIT2 | BIT3);             // clear everything in P1 except P1.2 and P1.3
    temp = number;                // move counterB to temp var
    temp &= ~(BIT2 | BIT3);             // clear just P1.2 and P1.3 in temp
    P1OUT |= temp;                      // output temp to port 1
}

/**
 * Computes the value to send to the LED
 * `LEDout` value will be sent to LED
 */
void LEDpattern() {
    int tempVar;                            // temporary variable

    //alternates between 2 led patterns when not cooling or heating
    if(Data == 'D') {

        if(patternCount == 0) {
            LEDout = 0b01010101;
            patternCount = 1;
        }
        else if(patternCount == 1) {
            LEDout = 0b10101010;
            patternCount = 0;
        }
    }

    else if(Data == 'B') {            // pattern for cooling

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

    else if(Data == 'A') {            // pattern for heating

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



#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void){
    UCB0IE &= ~UCRXIE0;
    Data = UCB0RXBUF;                   // receive data and assign it to `Data` variable
    if(Data == 'D') {
        patternCount = 0;                   // reset pattern
    }
    UCB0IE |= UCRXIE0;
}

