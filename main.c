#include <stdint.h>
#include "SysTickInts.h"
#include "LCD.h"
#include "msp432p401r.h"
#include "ADC14.h"
double tmp = 2/(16.384);
//uint8_t ADC_Val () {
  //  return SysTick_MailBox() * tmp;
//}

void main() {

    SysTick_Init(75000);

    ADC0_InitSWTriggerCh0();
    LCD_Init();


    LCD_OutString("Measuring: ");
        SysTick_Wait(3000000);
    while(1) {

        LCD_OutCmd(0xC0);
        //LCD_OutUDec(ADC_In());
        LCD_OutUFix(ADC_In()*tmp);
        LCD_OutString(" cm");

        P1OUT ^= 0x01;


    }
}

