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

/////////////////////////////////////////////////////////////
//Set the appropriate config bits
#define __CONFIG           0x2007
__code unsigned int __at (__CONFIG) cfg0 =  _CP_OFF & _CPD_OFF & _BOREN_OFF & _WDTE_OFF & _MCLRE_OFF & _FOSC_INTRCCLK;
////////////////////////////////////////////////////////////

//#define FOSC 8000000L

//prototypes
volatile unsigned int gTimerTick = 0x00;
void Delay(unsigned int val);
void GPIO_init(void);
void Timer0_init(void);
void ClockConfig(unsigned long hz);
void ClockTune(unsigned char value);


////////////////////////////////////////
//Interrupt Service Routine
//number following inerrupt keyword
//is the isr number.  there is only one
//interrupt for the pic, so set to 0
static void irqHandler(void) __interrupt 0
{
    //test the timer 0 if
    if (T0IF == 1)
    {
        gTimerTick++;
        T0IF = 0;
    }
}



int main()
{
    ClockConfig(8000000);   //125, 250hz, 500, 1000hz
    GPIO_init();
    Timer0_init();
   
    while (1)
    {
        PORTC ^= (1 << 0);
        Delay(100);
    }

    return 0;
}



////////////////////////////////////
//Generic delay function.  Not calibrated
//this was just to get the quad-core xmas 
//tree up and running
//
void Delay(unsigned int val)
{
    //reset the global timetick - increments
    //in the timer 0 isr    
    gTimerTick = 0x00;
    while(gTimerTick < val){};
}



////////////////////////////////////
//RC0 as output
//see Table 5 in datasheet for ANSEL bit
//to clear, clear all for now.
void GPIO_init(void)
{
    PORTC &=~ 0x01;     //initial value
    TRISC &=~ 0x01;     //config as output

    //configure ra4 as output - clkout
    TRISA &=~ (1u << 4);

    ANSEL = 0x00;     //set as digital
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
    TMR0 = 0x00;        //clear the timer

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

    OPTION_REG &=~ (1u << 5);       //timer mode
    OPTION_REG |= (1u << 4);        //high-low, dont care, counter only
    OPTION_REG &=~ (1u << 3);       //prescale timer 0
    OPTION_REG &=~ 0x07;            //clear bits 0-2

    OPTION_REG |= 0x02;            //prescale 8 = about 980hz

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



