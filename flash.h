#ifndef __FLASH_H__
#define __FLASH_H__

#define FLASH_INIT_ADDR 0x1000

void flashWb(unsigned char *, unsigned char);
void flashWbSegA(unsigned char *, unsigned char);
void flashWW(unsigned int *,unsigned int);
void flashClr(unsigned int *);
void flashWriteBuff(unsigned char *, unsigned char *, unsigned char);

#endif