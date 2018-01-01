/*
12/26/17
Dana Olcott

Simple Timer Program for the PIC16F690
Compiled with SDCC and flashed with the pickit3

The purpose of this program is to implement timer1
as a time keeper.  This will get combined with 
timer0 configured as counter to create a simple
frequncey counter.  No need to configure 
interrupts because the interrupt occurs on the
counter side.

Pin configs:
RC0-RC3 output digital
RA2 - counter input

Add a usart:
RB5 - RX - pin 12
RB7 - TX - pin 10

from pile counter code.

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
#define COUNTER_TRIGGER   (unsigned char)10     //value to trigger an interrupt
//load tmr0 reg with this value
#define COUNTER_RESET     (unsigned char)(0xFF - COUNTER_TRIGGER + 1)

//multiplier for prescale 8 and counter trigger
#define PRESCALE_8         ((unsigned long)250000)
#define PRESCALE_4         ((unsigned long)500000)
#define PRESCALE_2         ((unsigned long)1000000)

#define PRESCALE           PRESCALE_8

#define FREQUENCY_FACTOR      (2 * COUNTER_TRIGGER * PRESCALE)


///////////////////////////////////////////////////
/*
Measurements and Settings

///////////////////////////////////////////////////////////////
Use the following for frequency from 100hz = 4000hz

counter# 10, prescale = 8, factor = 250000
f = 2 * counter# * factor / tim1

freq = 2 * trigger * 250000 / timer1cnt     //timer1 prescale = 8
//////////////////////////////////////////////////////////////


Suitable Freq Ranges:
using the following settings:
counter# 10, prescale = 8, factor = 250000
f = 2 * counter# * factor / tim1

freq(scope)     tim1        freq(computed)
3500            1400        3571
208             24241       206
114             44366       112




using prescale = 8, counter = 1 ans looking
at lower frequencies.  added a check for
tmr1if - overflow... it works.!!

conclusion - good down to about 5hz

24.5            XX          24
12.5            xx          12
6.3             XX          32 - bad (ok with tmr1if check)
8.33            xx          8 - ok 
2.5             xx          7 - bad!!
5               xx          4 - ??

overflows. wonder if you can test the flag??
so can probably get down to about 5hz
using the tmr1if



//try another settin - 20 x 2 samples
//prescale = 8, see what freq. range we can get
4000khz looks ok, can't generate it any faster.


*/



//////////////////////////////////////
//prototypes
volatile unsigned int gTimerTick = 0x00;
unsigned int gCycleCounter = 0x00;

volatile unsigned char gActiveFrequency = 1;
volatile unsigned long gFrequency1 = 0x00;
volatile unsigned long gFrequency2 = 0x00;
volatile unsigned long gTimer1Value = 0x00;


void Delay(unsigned long val);
void GPIO_init(void);
void Timer0_init(void);
void ClockConfig(unsigned long hz);
void ClockTune(unsigned char value);

void Timer1_init(void);
void Timer1_start(void);
void Timer1_stop(void);
void Timer1_reset(void);
void Timer1_setValue(unsigned int value);

unsigned int Timer1_getValue(void);
unsigned long Timer1_getFrequency(void);
unsigned long gFreq;

unsigned char dec2Buff(unsigned long val, char* buffer);
void USART_init(void);
void USART_Write(char* buffer, unsigned char length);
void USART_WriteString(const char* buffer);


////////////////////////////////////
//usart output
unsigned char n;
#define OUT_BUFFER_SIZE     32
char outbuffer[OUT_BUFFER_SIZE];


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
    //interrupt soucre = timer0
    if (T0IF == 1)
    {

#ifdef USE_COUNTER
        
        //compute input frequency on RA2  
        gTimer1Value = Timer1_getValue() & 0xFFFF;

        //check timer1 overflow flag
        if (TMR1IF == 1)
            gTimer1Value += 0xFFFF;

        if (!gTimer1Value)      //avoid / 0
            gTimer1Value = 1;

        //FREQUENCY_FACTOR accounts for num cycles and
        //timer1 prescaler.
        if (gActiveFrequency == 1)
        {
            gFrequency2 = FREQUENCY_FACTOR / gTimer1Value;     //prescale2
            gActiveFrequency = 2;
        }
        else
        {
            gFrequency1 = FREQUENCY_FACTOR / gTimer1Value;     //prescale2
            gActiveFrequency = 1;
        }
        
        //flash debug led to indicate polling rate
        //init counter value to roll over at specified rate
        PORTC ^= (1u << 1);     //toggle RC1
        TMR0 = COUNTER_RESET;   //load the initial count value

        //reset timer and clear overflow flag
        Timer1_reset();

#else
        gTimerTick++;
#endif
        T0IF = 0;       //clear the counter flag
    }


   
}


/////////////////////////////////////
int main()
{
    ClockConfig(8000000);   //125, 250hz, 500, 1000hz
    GPIO_init();
    Timer0_init();
    Timer1_init();
    USART_init();

    while (1)
    {
        //status led
        PORTC ^= (1 << 0);

        //toggle RC3 and connect to RA2
        //RA2 is the input pin to counter.
        //RC3 is connected to pin 7
        //RA2 is pin 17
        PORTC ^= (1 << 3);

        //output the value over usart every 100 cycles
        if (!(gCycleCounter % 100))
        {
            USART_WriteString("Freq: ");
            gFreq = Timer1_getFrequency();
            n = dec2Buff(gFreq, outbuffer);

            USART_Write(outbuffer, n);
            USART_WriteString("hz\r\n");
        }

        Delay(10);
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

    //RA2 as counter source input
    //set as input and disable any pullup/down
    //any pullup/down
    TRISA |= (1u << 2);             //config as input   
    WPUA &=~ (1u << 2);             //disable weak pullup

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
    OSCTUNE = value;
}


/////////////////////////////////
//Timer1_init
//16bit timer/counter values: TMR1H and TMR1L
//clock source - T1CON - bit TMR1CS (0=internal fosc/4, 1 = external)
//timer enable - TMR1ON = 0 - disable
//prescale = 1:2 T1CON reg bits 5-4 
//
//timer tick rate should be fos/4 / 2 = 1mhz
//
void Timer1_init(void)
{
    //reset the timer count values
    TMR1H = 0x00;
    TMR1L = 0x00;

    //T1CON register
    T1CON &=~ (1u << 7);    //timer1 gate active low - NA
    T1CON &=~ (1u << 6);    //timer1 is on regardless of gate

  //  T1CON &=~ (1u << 5);    //prescale 01 - 1:2
  //  T1CON |= (1u << 4);     //prescale 01 - 1:2

    T1CON |= (1u << 5);    //prescale 11 - 1:8
    T1CON |= (1u << 4);     //prescale 11 - 1:8

    T1CON &=~ (1u << 3);    //LP is off
    T1CON |= (1u << 2);     //no sync ext clock - ok
    T1CON &=~ (1u << 1);    //internal clock - fosc/4

    TMR1IF = 0x00;          //timer1 overflow flag

    T1CON |= (1u << 0);     //timer1 is on

}


//////////////////////////////
//Start timer tick - bit 0 T1CON
void Timer1_start(void)
{
    T1CON |= 0x01; //enable
}

//////////////////////////////
//Stop timer1 - bit 0 low T1CON
void Timer1_stop(void)
{
    T1CON &=~ 0x01;    //disable
}

///////////////////////////////
//reset timer value - 16 bit in
//registers TMR1H and TMR1L
void Timer1_reset(void)
{
    T1CON &=~ 0x01;    //disable
    TMR1L = 0x00;
    TMR1H = 0x00;    
    TMR1IF = 0x00;    //clear ov flag
    T1CON |= 0x01;    //enable
}


//////////////////////////////////////
//set timer value
//sopt, set, resume
void Timer1_setValue(unsigned int value)
{
    T1CON &=~ 0x01;             //disable
    TMR1L = value & 0xFF;       //low byte
    TMR1H = (value >> 8) & 0xFF;//high byte
    TMR1IF = 0x00;              //clear ov flag
    T1CON |= 0x01;              //enable
}


//////////////////////////////////
//Return current 16bit timer1 value
unsigned int Timer1_getValue(void)
{
    unsigned int valuel, valueh;

    T1CON &=~ 0x01;    //disable
    valuel = TMR1L;
    valueh = TMR1H;
    T1CON |= 0x01;    //enable

    return ((valuel & 0xFF) + ((valueh & 0xFF) << 8));
}


unsigned long Timer1_getFrequency(void)
{
    if (gActiveFrequency == 1)
        return gFrequency1;
    else
        return gFrequency2;
}




////////////////////////////////////////////////
//set up the serial transimitter - much of this is from
//section 12.1 of the user manual
	
//setup the baud rate - use SPBRGH, SPBRG, BRGH, and BRG16 bits
//see section 12.3
//See Table 12-5.  For 8 mhz clock, 8 bit async, SYNC = 0
//BRGH = 1, and BRG16 = 0, 9600 baud, SPBRG = 51

void USART_init(void)
{
    //baud rate
	SPBRGH = 51;
	SPBRG = 51;

	//8 bit async mode - see table 12-3
	SYNC = 0;       //enable async opperation
	BRGH = 1;
	BRG16 = 0;

	//enable the serial port - clear the SYNC bit and 
	//set the SPEN bit.  Clearing the SYNC 
	SYNC = 0;       //enable async opperation
	SPEN = 1;       //auto config tx/rx pins as output/input

}



//////////////////////////////////////////////
//Serial Write
//buffer and  a length
//
void USART_Write(char* buffer, unsigned char length)
{
    unsigned char i = 0;

	//clear the TXREG
	TXREG = 0x00;
	TXEN = 1;

	for (i = 0 ; i < length ; i++)
	{
		TXEN = 1;
		TXREG = buffer[i];
		while (TRMT != 1)
		{}
	}

	TXEN = 0;   //disable tx to avoid bad data
}



///////////////////////////////////////////////
//
void USART_WriteString(const char* buffer)
{
    unsigned char i = 0;

	//clear the TXREG
	TXREG = 0x00;
	TXEN = 1;

    while ((buffer[i] != 0x00) && (i < 64))
    {
		TXEN = 1;
		TXREG = buffer[i];
		while (TRMT != 1)
		{}
        i++;
    }

	TXEN = 0;
}



///////////////////////////////////////////
//convert unsigned long value into a char
//buffer with return value of num chars to 
//print.
//For now, assume that is upto 10 000 000 - 8 chars

//keep getting warnings about left operand too
//short for reuslt regarding data in c or buffer
//
unsigned char dec2Buff(unsigned long val, char* buffer)
{
    unsigned char i = 0;
    char digit;
    unsigned char num = 0;
    char t;

    while (val > 0)
    {
        digit = (char)(val % 10);
        buffer[num] = (0x30 + digit) & 0x7F;
        num++;          
        val = val / 10;
    }

    //reverse in place
    for (i = 0 ; i < num / 2 ; i++)
    {
        t = buffer[i];
        buffer[i] = buffer[num - i - 1];
        buffer[num - i - 1] = t;
    }  

    buffer[num] = 0x00;     //null terminated
    
    return num;
}







