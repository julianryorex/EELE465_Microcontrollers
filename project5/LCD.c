#include <msp430.h>
unsigned char Data=0x37;
int i, j;
unsigned char temp;
int count = 0;

/**
 * Delay loop
 * k = number of ms
 */
void delay(int k){      //delay loop with 1ms inner loop and custom outer loop
    for(j=0; j<k; j++){
        for(i=0; i<102; i++){}
    }
}

/**
 * Latch function
 * (a way to tell the LCD whether the command sent is done)
 */
void Nibble(){  //enable on, delay, enable off
    P2OUT |= BIT7;
    delay(1);
    P2OUT &= ~BIT7;

}

/**
 * Sends command to LCD.
 * Check Datasheet for specific LCD commands
 */
void commSend(unsigned char cmd){
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
    delay(50);
}

/**
 * Sends character `dat` to LCD
 */
void dataSend(unsigned char dat){
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

/**
 * Reset the LCD mode
 * Clear display + set initial display to LCD
 */
void reset(){
    count = 0;
    commSend(0x01);         // clear display + start at position 0
    dataSend(0x45);
    dataSend(0x6E);
    dataSend(0x74);
    dataSend(0x65);
    dataSend(0x72);
    dataSend(0x20);
    dataSend(0x6E);
    dataSend(0x3A);
    commSend(0xC0);
    dataSend(0x54);
    dataSend(0x20);
    dataSend(0x3D);
    commSend(0xC7);
    dataSend(0xDF);
    dataSend(0x4B);
    commSend(0xCE);
    dataSend(0xDF);
    dataSend(0x43);
    commSend(0xC4);
}

/**
 * Initialize LCD to 4-bit mode
 * enable 2 line mode
 */
void init(){
    delay(100);
    P1OUT |= BIT5;          // setup 4-bit mode
    P1OUT |= BIT4;
    delay(30);
    Nibble();
    delay(10);
    P1OUT &= ~BIT4;         // clear the output for the mode
    P1OUT &= ~BIT5;
    Nibble();
    delay(10);
    Nibble();
    delay(10);
    Nibble();
    commSend(0x28);         // 4-bit 2-line mode
    commSend(0x10);         // Set cursor to first position
    commSend(0x0F);         // Display on; blinking cursor
    commSend(0x06);         // Entry mode set
    reset();
}

/**
 * main function (infinite loop)
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    UCB0CTLW0 |= UCSWRST;       // RESET

    UCB0CTLW0 |= UCMODE_3;      // SELECT I2C
    UCB0CTLW0 &= ~UCMST;        // slave MODE
    UCB0I2COA0 = 0x0014;        //SLAVE ADDRESS
    UCB0I2COA0 |= UCOAEN;       // slave address enable
    UCB0CTLW0 &= ~UCTR;         //RECEIVE MODE
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

    UCB0IE |= UCRXIE0;          //ENABLE I2C RX IRQ
    __enable_interrupt();

    while(1){}                  // infinite loop

    return 0;
}

#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) {
    UCB0IE &= ~UCRXIE0;
    Data = UCB0RXBUF; //Retrieve Data

    if(Data == 0x2A){               // when '*' is received
        reset();
    }
    else {
        if(count == 7) {            // received all data from master so go back to position 1
            count = 0;              // reset count global variable
            delay(200);             // delay
            commSend(0xC4);         //address for kelvin
        }
        else if(count == 3) {       // once kelvin data is received:
            commSend(0xCA);         // goto address for next line (for celsius)
            count++;
            dataSend(Data);         // send character to LCD
        }
        else {
            dataSend(Data);
            count++;
        }
    }

    for(i=0;i<1000;i++) {}
    UCB0IE |= UCRXIE0;
}

