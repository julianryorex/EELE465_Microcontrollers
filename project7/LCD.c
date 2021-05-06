#include <msp430.h>

// constants
const unsigned int packetLength = 5;

// global variables
int count = 0;                  // count current position in LCD display


// functions
void commSend(unsigned char);
void dataSend(unsigned char);
void reset();
void init();
void winnerState(unsigned int);


// helper functions
void delay(int);
void Nibble();




int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    UCB0CTLW0 |= UCSWRST;       // RESET

    UCB0CTLW0 |= UCMODE_3;      // SELECT I2C
    UCB0CTLW0 &= ~UCMST;        // slave MODE
    UCB0I2COA0 = 0x0014;        // SLAVE ADDRESS
    UCB0I2COA0 |= UCOAEN;       // slave address enable
    UCB0CTLW0 &= ~UCTR;         // RECEIVE MODE
    UCB0CTLW1 &= ~UCSWACK;      // auto acknowledge
    UCB0TBCNT = 0x01;           // Length of Receiving data

    UCB0CTLW1 &= ~UCTXIFG0;

    P1SEL1 &= ~BIT3;            // P1.3 = SCL
    P1SEL0 |= BIT3;

    P1SEL1 &= ~BIT2;            // P1.2 = SDA
    P1SEL0 |= BIT2;

    //Data ports for 4-bit mode
    P1DIR |= BIT7;
    P1DIR |= BIT6;
    P1DIR |= BIT5;
    P1DIR |= BIT4;

    P1OUT &= ~BIT7;
    P1OUT &= ~BIT6;
    P1OUT &= ~BIT5;
    P1OUT &= ~BIT4;

    P2DIR |= BIT6;              // RS port
    P2OUT &= ~BIT6;

    P2DIR |= BIT7;              // Enable Port
    P2OUT &= ~BIT7;

    PM5CTL0 &= ~LOCKLPM5;       // TURN ON DIGITAL I/O
    init();

    UCB0CTLW0 &= ~UCSWRST;      // OUT OF RESET

    UCB0IE |= UCRXIE0;          // ENABLE I2C RX IRQ

    __enable_interrupt();

    while(1){}                  // infinite loop

    return 0;
}









void winnerState(unsigned int w) {
    char winner = (w == 1) ? 0x31 : 0x32;

    commSend(0x85); // put cursor back to original location

    dataSend(0x50); // P
    dataSend(winner); // 1
    dataSend(0x20); // space
    dataSend(0x57); // W
    dataSend(0x4F); // O
    dataSend(0x4E); // N

    commSend(0xC0); // put cursor back to original location
    dataSend(0x50); // P
    dataSend(0x52); // R
    dataSend(0x45); // E
    dataSend(0x53); // S
    dataSend(0x53); // S

    dataSend(0x20); // space
    dataSend(0x2A); // *
    dataSend(0x20); // space

    dataSend(0x54); // T
    dataSend(0x4F); // O
    dataSend(0x20); // space

    dataSend(0x50); // P
    dataSend(0x4C); // L
    dataSend(0x41); // A
    dataSend(0x59); // Y
    dataSend(0x7E); // ->

}

void init() {

    delay(100);
    P1OUT |= BIT5;      // setup 4bit mode
    P1OUT |= BIT4;
    delay(30);
    Nibble();
    delay(10);
    P1OUT &= ~BIT4;     // clear the output for the mode
    P1OUT &= ~BIT5;
    Nibble();
    delay(10);
    Nibble();
    delay(10);
    Nibble();
    commSend(0x28);     // 4bit 2line mode
    commSend(0x10);     // Set cursor
    commSend(0x0F);     // Display on; blinking cursor
    commSend(0x06);     // Entry mode set

    reset();

}

void reset() {

    commSend(0x01); // clear display
    commSend(0xC0); // jump to second line
    dataSend(0x50); // P
    dataSend(0x31); // 1

    commSend(0x86); // jump to second line
    dataSend(0x30); // 0
    dataSend(0x20); // space
    dataSend(0x6D); // m
    dataSend(0x69); // i
    dataSend(0x6E); // n

    commSend(0xC5); // jump to second line
    dataSend(0x30); // 0
    dataSend(0x20); // space
    dataSend(0x2D); // -
    dataSend(0x2D); // -
    dataSend(0x20); // space
    dataSend(0x30); // 0

    commSend(0xCE); // jump to end of second line
    dataSend(0x50); // P
    dataSend(0x32); // 2

    commSend(0x85); // put cursor back to original location

}

void commSend(unsigned char cmd) {
    unsigned char temp;

    P1OUT &= 0x0F;      //clear output
    temp = cmd;
    temp &= 0xF0;       //clear lower nibble of data
    P1OUT |= temp;      // send upper nibble
    P2OUT &= ~BIT6;     //command mode
    Nibble();           //enable delay
    temp = cmd;
    temp= temp<<4;      //move lower nibble up
    temp &= 0xF0;       //clear all but lower nibble data
    P1OUT &= 0x0F;      //clear output
    P1OUT |= temp;      //send lower nibble
    Nibble();           //enable delay
    delay(20);

}

void dataSend(unsigned char dat) {
    unsigned char temp;

    P1OUT &= 0x0F;          //clear output
    temp = dat;
    temp &= 0xF0;           //clear lower nibble of data
    P1OUT |= temp;          //send upper nibble
    P2OUT |= BIT6;          //data mode
    Nibble();               //enable delay
    temp = dat;
    temp = temp << 4;       //move lower nibble up
    temp &= 0xF0;           //clear all but lower nibble data
    P1OUT &= 0x0F;          //clear output
    P1OUT |= temp;          //send lower nibble
    Nibble();               //enable delay
    delay(20);

}


void delay(int k){      //delay loop with 1ms inner loop and custom outer loop
    int i, j;

    for(j=0; j<k; j++){
        for(i=0; i<102; i++){}
    }
}

void Nibble(){  //enable on, delay, enable off
    P2OUT |= BIT7;
    delay(1);
    P2OUT &= ~BIT7;

}



#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) {

    UCB0IE &= ~UCRXIE0;
    char Data = UCB0RXBUF;                 // Retrieve Data

    if(Data == 0x2A) {                              // Reset code resets LCD
        reset();
        count = 0;
    }

    else if(Data == '[') {                          // player 1 won
        winnerState(1);
        count = 0;
    }

    else if(Data == ']') {                          // player 2 won
        winnerState(2);
        count = 0;
    }

    else {


        if(count == 0 || count == packetLength) {
            count = 0;
            commSend(0x85);
            dataSend(Data);
        }

        else if(count == 2) {
            commSend(0xC5);
            dataSend(Data);
            commSend(0xCA);
        }

        else if(Data == '!') {}                    // do nothing

        else {
            dataSend(Data);
        }

        count++;
    }

    delay(200);
    UCB0IE |= UCRXIE0;
}

