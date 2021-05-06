#include <msp430.h> 
char Data;
char num[2];
unsigned int globalCount = 0;               // global count for receiving data via I2C from master
unsigned int end = 1;


// implicit declaration of functions
void displayScore();
void displayEndState();
void reset();


// helper functions
void _turnOnAllLEDs();
void _turnOffAllLEDs();


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

    UCB0CTLW0 |= UCMODE_3;          //SELECT I2C
    UCB0CTLW0 &= ~UCMST;            //slave MODE
    UCB0I2COA0 = 0x0012;            //SLAVE ADDRESS
    UCB0I2COA0 |= UCOAEN;           // slave address enable
    UCB0CTLW0 &= ~UCTR;             //RECEIVE MODE
    UCB0CTLW1 &= ~UCSWACK;          // auto acknowledge
    UCB0TBCNT = 0x01;               // Length of Receiving data


    UCB0CTLW1 &= ~UCTXIFG0;         // TODO: check if this is needed after I2C integration

    P1SEL1 &= ~BIT3;                //P1.3 = SCL
    P1SEL0 |= BIT3;

    P1SEL1 &= ~BIT2;                //P1.2 = SDA
    P1SEL0 |= BIT2;

    // Ports 1.0-1.1, 1.4-1.7 are Bits 5-0
    // initialize as output
    P1DIR |= BIT7;
    P1DIR |= BIT6;
    P1DIR |= BIT5;
    P1DIR |= BIT4;
    P1DIR |= BIT1;
    P1DIR |= BIT0;

    //P2.7-2.6 are BITS 7-6
    // initialize as output
    P2DIR |= BIT7;
    P2DIR |= BIT6;

    // initialize all LED P1 ports as OFF
    P1OUT &= ~BIT7;
    P1OUT &= ~BIT6;
    P1OUT &= ~BIT5;
    P1OUT &= ~BIT4;
    P1OUT &= ~BIT1;
    P1OUT &= ~BIT0;

    // initialize all P2 ports as OFF
    P2OUT &= ~BIT7;
    P2OUT &= ~BIT6;

    PM5CTL0 &= ~LOCKLPM5;           // TURN ON DIGITAL I/O
    UCB0CTLW0 &= ~UCSWRST;          // OUT OF RESET

    UCB0IE |= UCRXIE0;              // ENABLE I2C RX IRQ
    __enable_interrupt();           // global enable

    reset();

    while(1){

        if(end == 0) {
            displayEndState();
        }

//        displayScore();
//        delay(2000);
//        displayEndState();
    }
    return 0;
}



/**
 * Determines the outputs of the LED ports based off of num
 * @params `num` is array[2] as {player1_score, player2_score}
 */
void displayScore() {
    switch(num[0]) {
        case '0':
            P2OUT &= ~BIT7;             // MSB
            P2OUT &= ~BIT6;             // LSB
            break;
        case '1':
            P2OUT |= BIT7;
            P2OUT &= ~BIT6;
            break;
        case '2':
            P2OUT |= BIT7;
            P2OUT |= BIT6;
            break;
    }

    switch(num[1]) {
        case '0':
            P1OUT &= ~BIT5;             // MSB
            P1OUT &= ~BIT4;             // LSB
            break;
        case '1':
            P1OUT &= ~BIT5;
            P1OUT |= BIT4;
            break;
        case '2':
            P1OUT |= BIT5;
            P1OUT |= BIT4;
            break;
    }
}

/**
 * Reset method to reset `num[]` and LED outputs
 */
void reset() {
    end = 1;
    globalCount = 0;
    num[0] = '2';
    num[1] = '2';
    _turnOffAllLEDs();
}


void displayEndState() {

    unsigned int index = 0;
    unsigned int winner = 0;

    if(num[0] == '0') {
        winner = 2;
    }

    else if(num[1] == '0') {
        winner = 1;
    }

    for(index; index < 4; index++) {
        _turnOnAllLEDs(0);
        delay(200);
        _turnOffAllLEDs();
        delay(200);
    }

    for(index = 0; index < 8; index++) {
        _turnOnAllLEDs(winner);
        delay(200);
        _turnOffAllLEDs();
        delay(200);
    }

    delay(1000);
}


// helper functions --------------------------------------------------------------------

void _turnOnAllLEDs(unsigned int player) {
    if(player == 0 || player == 1) {
        P2OUT |= BIT7;
        P2OUT |= BIT6;
        P1OUT |= BIT1;
        P1OUT |= BIT0;

    }
    if(player == 0 || player == 2) {
        P1OUT |= BIT7;
        P1OUT |= BIT6;
        P1OUT |= BIT5;
        P1OUT |= BIT4;
    }
}

void _turnOffAllLEDs() {
    P1OUT &= ~BIT7;
    P1OUT &= ~BIT6;
    P1OUT &= ~BIT5;
    P1OUT &= ~BIT4;
    P2OUT &= ~BIT7;
    P2OUT &= ~BIT6;
    P1OUT &= ~BIT1;
    P1OUT &= ~BIT0;

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

// -----------------------------------------------------
// ISR -------------------------------------------------
// -----------------------------------------------------
#pragma vector = EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) {
    UCB0IE &= ~UCRXIE0;                 // disable local enable
    Data = UCB0RXBUF;

    if(UCB0RXBUF == '*') {              // if reset symbol was sent,
        end = 1;
        reset();                        // reset LCD
    }

    else if(UCB0RXBUF == '0' || UCB0RXBUF == '1' || UCB0RXBUF == '2') {

        if(globalCount == 1) {
            num[1] = UCB0RXBUF;         // player 2

            if(num[0] == '0' || num[1] == '0') {
                end = 0;
            }
            else {
                displayScore();
            }

            globalCount = 0;

        }

        else {
            num[0] = UCB0RXBUF;         // player 1
            globalCount++;
        }
    }


    UCB0IE |= UCRXIE0;                  // enable local enable
}


