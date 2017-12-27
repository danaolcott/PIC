/*
12/25/17
Dana Olcott

Simple Timer Program for the PIC16F690
Compiled with SDCC and flashed with the pickit3

The purpose of this program is to implement timer0
as a timer and as a counter.  The timer is configured 
to interrupt on overflow.  Counter configuration
uses RA2 as the counter source, configured
as a high-to-low trigger.

Procedure:

Registers of interest:
TMR0 - clear timer register - jump as needed for
small adjustments in overflow rate.

OPTION reg - bit T0CS - set to 0 for timer mode
RA2 - configure as T0CKI for counter, set T0CS 1 for counter mode.

OPTION_REG
bit 5- TOCS - 0 = timer, 1 = counter
bit 4 - high, counter only, increment on high to low transition
bit 3 - PSA = 0  prescale assigned to timer 0
bits 0-2 = prescale value = 000 for prescale = 2

INTCON - interrupt config reg
bit 7 - GIE - global interrupt
bit 5 - T0IE - timer 0 interrupt enable
bit 2 - T0IF - interrupt flag

RA2 - config as T0CKI
TRISA - bit 2 - config as the input pin for counter.

All interrupts - pc is loaded with 0x0004h. SDCC
uses the following for isr with single interrupt
value:

static void irqHandler(void) __interrupt 0
{}

*/

#include <pic16f690.h>

////////////////////////////////////////////////
//Set the appropriate config bits
#define __CONFIG           0x2007
__code unsigned int __at (__CONFIG) cfg0 =  _CP_OFF & _CPD_OFF & _BOREN_OFF & _WDTE_OFF & _MCLRE_OFF & _FOSC_INTRCCLK;
////////////////////////////////////////////////


////////////////////////////////////////////////
//configure as timer or counter
//define counter ro run as counter
//else, run as timer
//Note: when using as counter, the number of cycles 
//to trigger an interrupt is 4 times the input
//pin toggle frequency.  i think this is due to 
//min prescale = 2 * 2 triggers to occur.  
//ie, for a setting =1, if input toggles at 10 hz
//the counter isr will toggle at 2.5 hz.
#define USE_COUNTER       1
#define COUNTER_TRIGGER   (unsigned char)1     //value to trigger an interrupt
//load tmr0 reg with this value
#define COUNTER_RESET     (unsigned char)(0xFF - COUNTER_TRIGGER + 1)



//////////////////////////////////////
//prototypes
volatile unsigned int gTimerTick = 0x00;
unsigned int gCycleCounter = 0x00;


void Delay(unsigned long val);
void GPIO_init(void);
void Timer0_init(void);
void ClockConfig(unsigned long hz);
void ClockTune(unsigned char value);


////////////////////////////////////////
//Interrupt Service Routine
//number following inerrupt keyword
//is the isr number.  there is only one
//interrupt for the pic, so set to 0
//
//whne configured as counter, it triggers
//on overflow.  
static void irqHandler(void) __interrupt 0
{
    //test the timer 0 if
    if (T0IF == 1)
    {
#ifdef USE_COUNTER
        
        //do something...
        PORTC ^= (1u << 1);     //toggle RC1
        TMR0 = COUNTER_RESET;   //load the initial count value
#else
        gTimerTick++;
#endif
        T0IF = 0;
    }
}


/////////////////////////////////////
int main()
{
    ClockConfig(8000000);   //125, 250hz, 500, 1000hz
    GPIO_init();
    Timer0_init();
   
    while (1)
    {
        //status led
        PORTC ^= (1 << 0);

        //toggle RC3 and connect to RA2
        //RA2 is the input pin to counter.
        //RC3 is connected to pin 7
        //RA2 is pin 17
        PORTC ^= (1 << 3);

        Delay(50);

        gCycleCounter++;
    }

    return 0;
}



//////////////////////////////////////////
//Delay function.
//When configured as counter, the delay function
//is not that accurate.  When configured as timer
//the delay function works well.
void Delay(unsigned long val)
{
#ifdef USE_COUNTER
    volatile unsigned long temp = val * 96;
    while (temp > 0)
        temp--;
#else
    gTimerTick = 0x00;
    while(gTimerTick < val){};
#endif

}



////////////////////////////////////
//RC0-RC3 as output
//
void GPIO_init(void)
{
    PORTC &=~ 0x0F;     //initial value
    TRISC &=~ 0x0F;     //config as output-c0-c3

    //configure ra4 as output - clkout
    TRISA &=~ (1u << 4);

    //config all io as digital
    ANSEL = 0x00;
    ANSELH = 0x00;
}


/////////////////////////////////
//Configure Timer0 as timer to run
//at 1khz.  Using the following settings
//and a clock config value of 8000000
//and tune value of 3, isr runs at 1000hz
//
void Timer0_init(void)
{
    //Registers of interest:
    //TMR0 - 8 bit reg - can write directly
#ifdef USE_COUNTER
    TMR0 = COUNTER_RESET;   //load the initial count value
#else
    TMR0 = 0x00;        //clear the timer
#endif

    //OPTION reg - bit T0CS - set to 0 for timer mode
    //RA2 - configure as T0CKI for counter, set T0CS 1 for counter mode.

    //OPTION reg 5-1
    //bit 5- low for timer, high for counter
    //bit 4 - high, counter only, increment on high to low transition
    //bit 3 - low = prescale assigned to timer 0
    //bits 0-2 = prescale value
    //000 = 2;
    //001 = 4;
    //010 = 8;
#ifdef USE_COUNTER
    OPTION_REG |= (1u << 5);       //counter mode
#else
    OPTION_REG &=~ (1u << 5);       //timer mode
#endif
    OPTION_REG |= (1u << 4);        //high-low, dont care, counter only
    OPTION_REG &=~ (1u << 3);       //prescale timer 0
    OPTION_REG &=~ 0x07;            //clear bits 0-2

#ifdef USE_COUNTER
    OPTION_REG |= 0x00;            //prescale 2 for counter
#else
    OPTION_REG |= 0x02;            //prescale 8 = about 980hz
#endif

    //interrupt flag
    INTCON |= (1u << 7);            //GIE enable
    INTCON |= (1u << 5);            //T0IE enable

    //clear the interrupt flag
    INTCON &=~ (1u << 2);           //flag T0IF

    //RA2 as counter source input
    //from 4-3, looks like this is
    //only one to set, other than disable 
    //any pullup/down
    TRISA |= (1u << 2);             //config as input   
    WPUA &=~ (1u << 2);             //disable weak pullup

}


/////////////////////////////////////
//Configure the internal oscillator 
//speed in hz.  For now, use, 1, 2, 4, 8mhz
//configure the OSCCON register.
//see page 50, reg 3-1 in datasheet
void ClockConfig(unsigned long hz)
{
    unsigned char clkBits = 0x00;
    switch(hz)
    {
        case 1000000:
            clkBits = (0x04 << 4);
            break;
        case 2000000:
            clkBits = (0x05 << 4);
            break;
        case 4000000:
            clkBits = (0x06 << 4);
            break;
        case 8000000:
            clkBits = (0x07 << 4);
            break;
        default:
            clkBits = (0x04 << 4);
            break;
    }

    clkBits |= 0x01;        //use internal osc as system clock
    OSCCON = clkBits;

    //minor adjustments in clock speed
    //falls on 125, 250, 500, 1000 hz
    ClockTune(3);
}


//////////////////////////////////////
//1 to 15 to increase freq.
//default 0
//31 to 16 to decrease
//no tune - output 980hz
//at 15   - output 1086

void ClockTune(unsigned char value)
{
    OSCTUNE = value;     //max
}



