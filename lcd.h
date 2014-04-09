#ifndef _LCD_H__
#define _LCD_H__

extern unsigned char lcdTicks;

#define PWM_PERIOD 16
#define PWM_PERIOD_ON 4

void lcdInit(void);
void lcdPutString(unsigned char *,unsigned char, unsigned char);
void lcdPutStringSearch(unsigned char *, unsigned char, unsigned char, unsigned char);
void lcdWait(void);
void lcdBacklightOn(void);
void lcdBacklightOff(void);
void lcdUpdate(void);
unsigned char lcdLowUpdate(void);
unsigned char lcdGetChar(unsigned char);
void lcdSetChar(unsigned char, unsigned char, unsigned char);

void lcdContrSet(void);
void lcdContrClear(void);


#endif