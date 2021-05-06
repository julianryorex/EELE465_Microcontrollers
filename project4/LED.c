#include <msp430.h> 
int Data;
int counterB = 0;
int counterC = 0x01;

// implicit declaration of functions
void patternB();
void patternC();
void _displayNumber(int number);
int delay(int longCount);
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
        if(Data==0x41){                 // static pattern A
            P1OUT &= (BIT2 | BIT3);     // clear everything in P1 except P1.2 and P1.3
            P2OUT &= 0b00000000;        // clear all P2
            P1OUT |= 0b10100010;
            P2OUT |= 0b10000000;
        }
        else if(Data==0x42) {           // B pattern
            patternB();
        }
        else if(Data==0x43) {           // C pattern
            patternC();
        }
    }
    return 0;
}

// -------------------------------------------------------------------------------------------------
/**
 * Displays Pattern B - Binary Counter
 */
void patternB() {
                                // increment counter
    _displayNumber(counterB);                      // display counter

    delay(600);                             // delay of 100ms
    if(counterB == 255) {
        counterB = 0;
    }
    else {
        counterB++;
    }
}

/**
 * Displays Pattern C - Binary Shifter
 */
void patternC() {
    _displayNumber(counterC ^ 0xFF);       // display counter

    delay(600);                     // delay of 100ms
    if(counterC == 128) {
        counterC = 0x01;               // reset to 0
    }
    else {
        counterC = counterC << 1;   // shift counter C
    }
}


/**
 * Ports used for counter:
 * P1DIR |= BIT7;
 * P1.0, P1.1, P1.4, P1.5, P1.6, P1.7, P2.6, P2.7
 */
void _displayNumber(int number) {
    // MSB 7     6     5     4     3     2     1     0     LSB
    // MSB P1.7  P1.6  P1.5  P1.4  P2.7  P2.6  P1.1  P1.0  LSB
    int temp;

    P1OUT &= (BIT2 | BIT3);             // clear everything in P1 except P1.2 and P1.3
    temp = number;                // move counterB to temp var
    temp &= ~(BIT2 | BIT3);             // clear just P1.2 and P1.3 in temp
    P1OUT |= temp;                      // output temp to port 1

    P2OUT &= ~(BIT6 | BIT7);            // clear everything in P2 except P2.6 and P2.7
    temp = number;                    // same thing with port 2, temp = counterB
    temp &= (BIT2 | BIT3);              // clear all except PX.2 and PX.3 (temp)
    temp = temp << 4;                   // shift left 4
    P2OUT |= temp;                      // output temp to P2

}



// delay - give it number of milliseconds
int delay(int longCount) {
    while(longCount > 0) {
        longCount--;
        _delay1ms();
    }
    return 0;
}

// delay for 1 ms (private function)
int _delay1ms() {
    int shortCount;
    for(shortCount = 0; shortCount < 102; shortCount++) {}
    return 0;
}

#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void){
    UCB0IE &= ~UCRXIE0;
    if(UCB0RXBUF != '\0') {
        Data = UCB0RXBUF;
    }
    UCB0IE |= UCRXIE0;
}

