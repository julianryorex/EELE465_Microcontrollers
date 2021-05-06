#include <msp430.h> 

char keypad;                    // character pressed from keypad
char slaveAddrLED = 0x0012;
char slaveAddrLCD = 0x0013;



int main(void) {
    WDTCTL = WDTPW | WDTHOLD;           // stop watchdog timer

    // Setup ports
    UCB1CTLW0 |= UCSWRST;               // put into software reset

    UCB1CTLW0 |= UCSSEL_3;              // choose SMCLK
    UCB1BRW = 10;                       // divide by 10 to get SCL = 100kHz

    UCB1CTLW0 |= UCMODE_3;              // put into I2C mode
    UCB1CTLW0 |= UCMST;                 // put into Master mode
    UCB1CTLW0 |= UCTR;                  // put into Tx/WRITE mode

    UCB1CTLW1 |= UCASTP_2;              // AUTO stop when UCB1CNT reached
    UCB1TBCNT = 0x01;                   // send 1 byte of data

    P4SEL1 &= ~BIT7;                    //Set P4.7 as SCL
    P4SEL0 |= BIT7;
    P4SEL1 &= ~BIT6;                    //Set P4.6 as SDA
    P4SEL0 |= BIT6;

   UCB1CTLW0 &= ~UCSWRST;               // put out of software reset

   PM5CTL0 &= ~LOCKLPM5;                //Turn on I/O

   P4DIR |= BIT1;                       // initialize unlock LED
   P4OUT &= ~BIT1;                      // initialize as OFF

   UCB1IE |= UCTXIE0;                   // Enable I2C TX0 IRQ (transmit reg is ready)
   __enable_interrupt();                // global enabl

   int i;
   int unlock = 0;                      // change back to 0

   // passcode
   while(unlock < 3) {
       keypad = checkKeypad();

       if(keypad == '4' && unlock == 0) {
           unlock++;
       }
       if(keypad == '3' && unlock == 1) {
           unlock++;
       }
       if(keypad == '7' && unlock == 2) {
           unlock++;
       }
   }

   while(1) {
       P4OUT |= BIT1;                           // Turn on LED to show it's unlocked

       keypad = checkKeypad();                  // check keypad value. Will return '\0' if nothing
       if(keypad != '\0') {
            UCB1I2CSA = slaveAddrLED;                 // slave address to 0x12 (slave address)
            UCB1CTLW0 |= UCTXSTT;               //Generate START, triggers ISR right after
            Delay(200);

//            UCB1I2CSA = slaveAddrLCD;     // slave address to 0x12 (slave address)
//            UCB1CTLW0 |= UCTXSTT;         //Generate START
//            Delay(200);
       }

   }

    return 0;
}

// checks the keypad
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

    return getCharacter(buttonPressed);
}

char getCharacter(char buttonPressed) {
    char button = '/0';

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
    }
    return button;
}

/*-----------------------------------------------------------
 * Delay1ms: Delay set for approx 1 ms
-------------------------------------------------------------*/
int Delay(int LongCount) {
    while(LongCount > 0) {
        LongCount--;
        Delay1ms();
    }
    return 0;
} //-----------------End Delay1ms ------------------------------------


/*-----------------------------------------------------------
 * Delay1ms: Delay set for approx 1 ms
-------------------------------------------------------------*/
int Delay1ms() {
    int ShortCount;
    for(ShortCount = 0; ShortCount < 102; ShortCount++) {}
    return 0;
}



// ISRs -------------------------------------------------------------
#pragma vector=EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void) {
    UCB1TXBUF = keypad;
}


