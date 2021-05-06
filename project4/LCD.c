#include <msp430.h>
unsigned char Data=0x37;
int i, j;
unsigned char temp;
int count;

void delay(int k){      //delay loop with 1ms inner loop and custom outer loop
    for(j=0; j<k; j++){
        for(i=0; i<102; i++){}
    }
}

void Nibble(){  //enable on, delay, enable off
    P2OUT |= BIT7;
    delay(1);
    P2OUT &= ~BIT7;

}
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
}

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
}

void init(){
    delay(100);
    P1OUT |= BIT5;  //setup 4bit mode
    P1OUT |= BIT4;
    delay(30);
    Nibble();
    delay(10);
    P1OUT &= ~BIT4;    //clear the output for the mode
    P1OUT &= ~BIT5;
    Nibble();
    delay(10);
    Nibble();
    delay(10);
    Nibble();
    commSend(0x28);     //4bit 2line mode
    commSend(0x10);     //Set cursor
    commSend(0x0F);     //Display on; blinking cursor
    commSend(0x06);     //Entry mode set
}
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

    //Data ports for 4-bit mode
    P1DIR |= BIT7;
    P1DIR |= BIT6;
    P1DIR |= BIT5;
    P1DIR |= BIT4;
    P1OUT &= ~BIT7;
    P1OUT &= ~BIT6;
    P1OUT &= ~BIT5;
    P1OUT &= ~BIT4;

    P2DIR |= BIT6;          //RS port
    P2OUT &= ~BIT6;

    P2DIR |= BIT7;  //Enable Port
    P2OUT &= ~BIT7;

    PM5CTL0 &= ~LOCKLPM5; // TURN ON DIGITAL I/O
    init();

    UCB0CTLW0 &= ~UCSWRST;  // OUT OF RESET

    UCB0IE |= UCRXIE0;      //ENABLE I2C RX IRQ

    __enable_interrupt();

    while(1){
/*        Data=0x39;
        if(count == 32){
                commSend(0x01); //clear display
                commSend(0x80);
                count=0;
            }else if(count == 16){
                commSend(0xC0); //address for next line
                count++;
                dataSend(Data);
            }else{
                dataSend(Data);
                count++;
            }
        delay(200);
*/    }
    return 0;
}

#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void){
    UCB0IE &= ~UCRXIE0;
    Data = UCB0RXBUF; //Retrieve Data
    if(count == 32){
        commSend(0x01); //clear display
        delay(200);
        count=1;
        dataSend(Data);
    }else if(count == 16){
        commSend(0xC0); //address for next line
        count++;
        dataSend(Data);
    }else{
        dataSend(Data);
        count++;
    }
    delay(200);
    UCB0IE |= UCRXIE0;
    }

