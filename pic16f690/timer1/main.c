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

Add a usart out of RB7 as a way of output the measured freq.
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
#define COUNTER_TRIGGER   (unsigned char)1     //value to trigger an interrupt
//load tmr0 reg with this value
#define COUNTER_RESET     (unsigned char)(0xFF - COUNTER_TRIGGER + 1)



//////////////////////////////////////
//prototypes
volatile unsigned int gTimerTick = 0x00;
unsigned int gCycleCounter = 0x00;

volatile unsigned char gActiveFrequency = 1;
volatile unsigned long gFrequency1 = 0x00;
volatile unsigned long gFrequency2 = 0x00;


void Delay(unsigned long val);
void GPIO_init(void);
void Timer0_init(void);
void ClockConfig(unsigned long hz);
void ClockTune(unsigned char value);

void Timer1_init(void);
void Timer1_start(void);
void Timer1_stop(void);
void Timer1_reset(void);
unsigned int Timer1_getValue(void);
unsigned long Timer1_getFrequency(void);

unsigned char dec2Buff(unsigned long val, char* buffer);
void USART_init(void);
void USART_Write(char* buffer, unsigned char length);
void USART_WriteString(const char* buffer);
void USART_ProcessCommand(unsigned char* buffer, unsigned char length);


unsigned char n;
#define OUT_BUFFER_SIZE     32
#define RX_BUFFER_SIZE      32
char outbuffer[OUT_BUFFER_SIZE];

//receiver variables
unsigned char rxIndex = 0x00;
unsigned char rxBuffer[RX_BUFFER_SIZE];

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
    unsigned char c = 0x00;

    //soucre = timer0 / counter0
    if (T0IF == 1)
    {

#ifdef USE_COUNTER
        
        //compute the tick freq and store
        //in non active freq and switch
        //freq = counter * 1000000 / ticks
        //
        unsigned long tick = Timer1_getValue();
        unsigned long counter = (unsigned long)COUNTER_TRIGGER * 2;

        if (!tick)
            tick = 1;

        if (gActiveFrequency == 1)
        {
            gFrequency2 = counter * 1000000 / tick;
            gActiveFrequency = 2;
        }
        else
        {
            gFrequency1 = counter * 1000000 / tick;
            gActiveFrequency = 1;
        }
        
        //do something...
        PORTC ^= (1u << 1);     //toggle RC1
        TMR0 = COUNTER_RESET;   //load the initial count value

        //reset timer1
        Timer1_stop();
        Timer1_reset();
        Timer1_start();
#else
        gTimerTick++;
#endif
        T0IF = 0;
    }


    ////////////////////////////////////////
    //receiver interrupt
    if (RCIF == 1)    
    {
        c = RCREG;

        if ((rxIndex < (RX_BUFFER_SIZE - 1)) && (c != 0x00))
        {
            rxBuffer[rxIndex] = c;
            rxIndex++;

            //end of message?
            if (c == '\n')
            {
                rxBuffer[rxIndex] = 0x00;
                USART_ProcessCommand(rxBuffer, rxIndex);
                rxIndex = 0x00;
            }
        }


        RCIF = 0;        
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
        if (!(gCycleCounter % 500))
        {
            USART_WriteString("Freq: ");

            n = dec2Buff(Timer1_getFrequency(), outbuffer);
            USART_Write(outbuffer, n);
            USART_WriteString("hz\r\n");
        }

        Delay(1);
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
    //from 4-3, looks like this is
    //only one to set, other than disable 
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
    T1CON &=~ (1u << 5);    //prescale 01 - 1:2
    T1CON |= (1u << 4);     //prescale 01 - 1:2
    T1CON &=~ (1u << 3);    //LP is off
    T1CON |= (1u << 2);     //no sync ext clock - NA
    T1CON &=~ (1u << 1);    //internal clock - fosc/4
    T1CON |= (1u << 0);     //timer1 is on

}


//////////////////////////////
//Start timer tick - bit 0 T1CON
void Timer1_start(void)
{
    T1CON |= (1u << 0); //enable
}

//////////////////////////////
//Stop timer1 - bit 0 low T1CON
void Timer1_stop(void)
{
    T1CON &=~ (1u << 0);    //disable
}

///////////////////////////////
//reset timer value - 16 bit in
//registers TMR1H and TMR1L
void Timer1_reset(void)
{
    TMR1H = 0x00;
    TMR1L = 0x00;
}

//////////////////////////////////
//Return current 16bit timer1 value
unsigned int Timer1_getValue(void)
{
    unsigned int valuel = TMR1L;
    unsigned int valueh = TMR1H;
    unsigned int val = (valuel & 0xFF) | ((valueh & 0xFF) << 8);

    return val;
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
    unsigned char c;

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

    //receiver - 12.1.2.1
    SYNC = 0;
    SPEN = 1;

    //config rx interrupts
    RCIE = 1;       //PIE1 register
    PEIE = 1;       //INTCON register
    GIE = 1;        //intcon register

    CREN = 1;       //enable the receiver

    
    //check this in the isr
    //RCIF - receiver interrupt flag
    c = RCREG;
    RCIF = 0x00;


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
unsigned char dec2Buff(unsigned long val, char* buffer)
{
    unsigned char i = 0;
    unsigned char digit;
    unsigned char num = 0;
    unsigned long temp = val;
    unsigned int c[10];

    for (i = 0 ; i < 10 ; i++)
    {
        if (temp >= 10)
        {
            digit = temp % 10;      //get the ones
            num++;
        }

        else if ((temp < 10) && (temp > 0))
        {
            digit = temp;
            num++;
        }

        else
            digit = 0;

        //get the acsii - base + digit
        c[9-i] = (unsigned char)(0x30 + digit);      //ascii value for the digit
        temp = temp / 10;
    }

    //copy c into buffer
    for (i = 0 ; i < 10 ; i++)
        buffer[i] = (unsigned char)c[i];

    
    return num;
}




///////////////////////////////////////
//
void USART_ProcessCommand(unsigned char* buffer, unsigned char length)
{
    if (length > 0)
    {
        if (buffer[0] == 'a')
            USART_WriteString("cmd: a\r\n");
        else if (buffer[0] == 'b')
            USART_WriteString("cmd: b\r\n");
        else if (buffer[0] == 'c')
            USART_WriteString("cmd: c\r\n");
        else if (buffer[0] == 'd')
            USART_WriteString("cmd: d\r\n");



    }    
}






