/*
12/24/17
Dana Olcott

Simple Flash Program for the PIC16F690
Compiled with SDCC and flashed with the pickit3

install package note:

install 64 bit version of sdcc
install libc6-pic
install gputils - linker scripts, pic utilities...


useful paths to include in your build script
I_PATH1="/usr/local/share/sdcc/include"
I_PATH2="/usr/local/share/sdcc/non-free/include/pic14"
I_PATH3="/usr/local/share/sdcc/lib/pic14"
I_PATH4="/usr/local/share/sdcc/non-free/lib/pic14"
I_PATH5="/usr/share/gputils/lkr/"



Program:  pic16f690
Pins of interest:

Pin     label
1       Vdd
20      Vss

Programmer:
PICKIT      PIC       Label
1           4       Vpp
2           NA      5v
3           GND     GND
4           19      RA0
5           18      RA1
6           NC      NC




__code unsigned int __at (__CONFIG) cfg0 =  _CP_OFF & _CPD_OFF & _BOREN_OFF & _WDTE_OFF & _MCLRE_OFF & _FOSC_INTRCCLK;
these all need to get and together
cp-off =            0x3FFF = 11 1111 1111 1111
cpd-off =           0x3FFF = 11 1111 1111 1111
brown off =         0x3CFF = 11 1100 1111 1111
watchdog off =      0x3FF7 = 11 1111 1111 0111
mclre off =         0x3FDF = 11 1111 1101 1111
fosc =              0x3FFD = 11 1111 1111 1101

            and-ed           11 1100 1101 0101 - Write this value to 0x2007

*/

#include <pic16f690.h>

//////////////////////////////////////////////
//Set the appropriate config bits
#define __CONFIG           0x2007
__code unsigned int __at (__CONFIG) cfg0 =  _CP_OFF & _CPD_OFF & _BOREN_OFF & _WDTE_OFF & _MCLRE_OFF & _FOSC_INTRCCLK;
//////////////////////////////////////////////


//prototypes
void Delay(unsigned int val);


//variables
static unsigned char counter = 0;

int main()
{
    //Configure PORTC as output and
    //set initial states as low
    PORTC = 0x00;       //initial value
    TRISC = 0x00;       //config as output
    ANSEL = 0x00;       //set as digital

    while (1)
    {
        if (!(counter%2))
            PORTC ^= (1 << 0);
        else if (!(counter%3))
            PORTC ^= (1 << 1);
        else if (!(counter%5))
            PORTC ^= (1 << 2);

        Delay(5000);
        counter++;
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
    volatile unsigned int temp = val;
    while(temp > 0)
        temp--;
}





