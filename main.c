#include <msp430g2553.h>

int i=0 ;
int prox = 0;
int initProx = 0;
int prox2 = 0;
int initProx2 = 0;
int ADCReading [2];
unsigned char a=0;

#define ROTATE_RESET    0
#define ROTATE_0        600
#define ROTATE_45       1125
#define ROTATE_90       1650
#define ROTATE_135      2175
#define ROTATE_180      2700


// Function Prototypes
void ConfigureAdc(void);
void getanalogvalues();
void serialInit();
unsigned char serialRead();
void serialWrite(unsigned char c);
void serialwriteString(const char *str);
void openDoor();

#define servo_rotate(arg1) \
    for (i=0; i<30; i++) {  \
        P2OUT |= BIT2;      \
        __delay_cycles(arg1);   \
        P2OUT &= ~BIT2;         \
        __delay_cycles(20000);  \
    }

int main(void)
{
    WDTCTL = WDTPW + WDTHOLD;                   // Stop WDT
    P1OUT = 0;
    P2OUT = 0;
    P1DIR = 0;
    P1REN = 0;
    P2REN = 0;
    P2DIR = 0;
    P1DIR |= (BIT0 | BIT6);             // set bits 4, 5, 6 as outputs
    P2DIR |= (BIT0 | BIT5 | BIT4 | BIT3 | BIT2);

    ConfigureAdc();
    serialInit();

    // reading the initial proximity values
    __delay_cycles(250);
    getanalogvalues();
    initProx = prox;
    initProx2 = prox2;
    __delay_cycles(250);


    for (;;)
    {
        //reading temp repeatedly at the beginning of the main loop
        getanalogvalues();

        //Temp Controlling a LED on port 2.0
        //The code below uses a smple comparision which create flickering
        //Modify the code below to create a dead zon to prevent fleckering
        //if prox is higher than 1.04 * initProx, turn LED2 on
        //if prox is lower  than 1.02 * initProx, turn LED2 off
        if(prox > initProx+150)
        {
            P2OUT = 0x00;
            P2OUT = 0x01;
            __delay_cycles(200);

            serialwriteString("Open Door?");
            __delay_cycles(200);

            a = serialRead();
            if(a== '0')             //Request denied
            {
                //P2OUT |=  BIT5;
                //__delay_cycles(500000);
                //P2OUT |=  ~BIT5;
            }
            else if (a== '1')       //Request accepted
            {
                openDoor();
            }
            P2OUT = 0x00;
        }    // SSR on
        if(prox2 > initProx2+150)
        {
            P2OUT = 0x00;
            P2OUT = 0x10;
            __delay_cycles(200);
            openDoor();
            P2OUT = 0x00;
        }
        else if(prox < initProx*1.05)
        {
            __delay_cycles(200);
        }    // SSR off
    }
}

void ConfigureAdc(void)
{
   ADC10CTL1 =   INCH_4 | CONSEQ_1;               // A2 + A1 + A0, single sequence
   ADC10CTL0 = ADC10SHT_2 | MSC | ADC10ON;
   while (ADC10CTL1 & BUSY);
   ADC10DTC1 = 0x03;                              // 3 conversions
   ADC10AE0 |= (BIT0 | BIT3);                             // ADC10 option select
}

void getanalogvalues()
{
 i = 0; prox = 0; prox2=0;                               // set all analog values to zero
  for(i=1; i<=5 ; i++)                          // read all three analog values 5 times each and average
  {
    ADC10CTL0 &= ~ENC;
    while (ADC10CTL1 & BUSY);                         //Wait while ADC is busy
    ADC10SA = (unsigned)&ADCReading[0];               //RAM Address of ADC Data, must be reset every conversion
    ADC10CTL0 |= (ENC | ADC10SC);                     //Start ADC Conversion
    while (ADC10CTL1 & BUSY);                         //Wait while ADC is busy
    prox += ADCReading[0];
    prox2 += ADCReading[1];
  }
  prox = prox/5;                                        // Average the 5 reading for the three variables
  prox2 = prox2/5;
}

void serialInit()
{
    P1SEL= BIT1 + BIT2; //P1.1 = RXD P1.2=TXD
    P1SEL2= BIT1 +BIT2; // P1.1=RXD & P1.2=TXD
    UCA0CTL1|= UCSSEL_2; // SMCLK
    UCA0BR0 = 0X08; // 1MHz 115200
    UCA0BR1 = 0;//0x00; // 1MHz 115200

    UCA0MCTL= UCBRS2 + UCBRS0; // MODULATION UCBRSx=1
    UCA0CTL1&=~UCSWRST; // ** INITIALIZE USCI STATE MACHINE
    IE2|= UCA0RXIE; // ENABLE VSCI_A0 RX INTERRUPT
}

unsigned char serialRead()
{
    while(!(IFG2&UCA0RXIFG));   //USCI_A0 RX buffer ready ?
    return UCA0RXBUF;
}

void serialWrite(unsigned char c)
{
    while(!(IFG2&UCA0TXIFG));  // USCI_A0 TX buffer ready ?
    UCA0TXBUF=c; // TX
}

void serialwriteString(const char *str)
{
    while(*str)
        serialWrite(*str++);
}

void openDoor()
{
    servo_rotate(ROTATE_0);
    servo_rotate(ROTATE_45);
    __delay_cycles(5000000);
    servo_rotate(ROTATE_0);
}


#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
    __bic_SR_register_on_exit(CPUOFF);
}
