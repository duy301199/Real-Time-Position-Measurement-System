/*
  size is 1*16
  if do not need to read busy, then you can tie R/W=ground
  ground = pin 1    Vss
  power  = pin 2    Vdd   +3.3V or +5V depending on the device
  ground = pin 3    Vlc   grounded for highest contrast
  P6.4   = pin 4    RS    (1 for data, 0 for control/status)
  ground = pin 5    R/W   (1 for read, 0 for write)
  P6.5   = pin 6    E     (enable)
  P4.4   = pin 11   DB4   (4-bit data)
  P4.5   = pin 12   DB5
  P4.6   = pin 13   DB6
  P4.7   = pin 14   DB7
16 characters are configured as 1 row of 16
addr  00 01 02 03 04 05 ... 0F
*/

#include <stdint.h>
#include "LCD.h"
#include "SysTickInts.h"
#include "msp.h"
#include <stdio.h>
// Marcros
#define BusFreq 3					// assuming a 3 MHz bus clock
#define T6us 6*BusFreq				// 6us
#define T40us 40*BusFreq			// 40us
#define T160us 160*BusFreq			// 160us
#define T1ms 1000*BusFreq			// 1ms
#define T1600us 1600*BusFreq		// 1.60ms
#define T5ms 5000*BusFreq			// 5ms
#define T15ms 15000*BusFreq			// 15ms

// Global Vars
uint8_t LCD_RS, LCD_E;				// LCD Enable and Register Select

/**************** Private Functions ****************/

void OutPort6() {
	P6OUT = (LCD_RS<<4) | (LCD_E<<5);
}

void SendPulse() {
	OutPort6();
	SysTick_Wait(T6us);				// wait 6us
	LCD_E = 1;						// E=1, R/W=0, RS=1
	OutPort6();
	SysTick_Wait(T6us);				// wait 6us
	LCD_E = 0;						// E=0, R/W=0, RS=1
	OutPort6();
}

void SendChar() {
	LCD_E = 0;
	LCD_RS = 1;						// E=0, R/W=0, RS=1
	SendPulse();
	SysTick_Wait(T1600us);			// wait 1.6ms
}

void SendCmd() {
	LCD_E = 0;
	LCD_RS = 0;						// E=0, R/W=0, RS=0
	SendPulse();
	SysTick_Wait(T40us);			// wait 40us
}

/**************** Public Functions ****************/
// Clear the LCD
// Inputs: none
// Outputs: none
void LCD_Clear() {
	LCD_OutCmd(0x01);				// Clear Display
	LCD_OutCmd(0x80);				// Move cursor back to 1st position
}

// Initialize LCD
// Inputs: none
// Outputs: none
void LCD_Init() {
    //Using P1 for heartbeat

        P1SEL0 &= ~0x01;
        P1SEL1 &= ~0x01;
        P1DIR  |= 0x01;

        P4SEL0 &= ~0xF0;
        P4SEL1 &= ~0xF0;                // configure upper nibble of P4 as GPIO
        P4DIR |= 0xF0;                  // make upper nibble of P4 out

        P6SEL0 &= ~0x30;
        P6SEL1 &= ~0x30;                // configure P6.4 and P6.5 as GPIO
        P6DIR |= 0x30;                  // make P6.4 and P6.5 out

        //Clock_Init48MHz();                // set system clock to 48 MHz
        SysTick_Init(1);                 // Volume 1 Program 4.7, Volume 2 Program 2.12

        LCD_E = 0;
        LCD_RS = 0;                     // E=0, R/W=0, RS=0
        OutPort6();

        LCD_OutCmd(0x30);               // command 0x30 = Wake up
        SysTick_Wait(T5ms);             // must wait 5ms, busy flag not available
        LCD_OutCmd(0x30);               // command 0x30 = Wake up #2
        SysTick_Wait(T160us);           // must wait 160us, busy flag not available
        LCD_OutCmd(0x30);               // command 0x30 = Wake up #3
        SysTick_Wait(T160us);           // must wait 160us, busy flag not available

        LCD_OutCmd(0x28);               // Function set: 4-bit/2-line
        LCD_Clear();
        LCD_OutCmd(0x10);               // Set cursor
        LCD_OutCmd(0x06);               // Entry mode set
}

// Output a character to the LCD
// Inputs: letter is ASCII character, 0 to 0x7F
// Outputs: none
void LCD_OutChar(char letter) {
    unsigned char let_low = (0x0F&letter)<<4;
    unsigned char let_high = 0xF0&letter;

    P4OUT = let_high;
    SendChar();
    P4OUT = let_low;
    SendChar();
    SysTick_Wait(T1ms);             // wait 1ms
}

// Output a command to the LCD
// Inputs: 8-bit command
// Outputs: none
void LCD_OutCmd(unsigned char command) {
    unsigned char com_low = (0x0F&command)<<4;
    unsigned char com_high = 0xF0&command;
    P4OUT = com_high;
    SendCmd();
    P4OUT = com_low;
    SendCmd();
    SysTick_Wait(T1ms);             // wait 1ms
}

//------------LCD_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void LCD_OutString(char *pt) {
    while(*pt)
      {
          LCD_OutChar(*pt);
          pt++;
      }
}

//-----------------------LCD_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void LCD_OutUDec(uint32_t n) {

    while (n >= 10){
    LCD_OutUDec(n/10);
    n = n%10;
    }

}

//--------------------------LCD_OutUHex----------------------------
// Output a 32-bit number in unsigned hexadecimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1 to 8 digits with no space before or after
void LCD_OutUHex(uint32_t number) {
    if (number >= 0x10) {
        LCD_OutUHex(number / 0x10);
        LCD_OutUHex(number % 0x10);

    }
    else {
        if(number < 0xA) {
            LCD_OutChar( number + '0');
        }
        else LCD_OutChar(( number - 0x0A) + 'A');
    }
}

// -----------------------LCD_OutUFix----------------------
// Output characters to LCD display in fixed-point format
// unsigned decimal, resolution 0.001, range 0.000 to 9.999
// Inputs:  an unsigned 32-bit number
// Outputs: none
// E.g., 0,    then output "0.000 "
//       3,    then output "0.003 "
//       89,   then output "0.089 "
//       123,  then output "0.123 "
//       9999, then output "9.999 "
//       9999, then output "*.*** "
void LCD_OutUFix(uint32_t number) {
    char array[12];
        double string = number / 1000.0;
        sprintf(array, "%.3lf", string);
        LCD_OutString(array);
}
