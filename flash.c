#include <intrinsics.h>
#include "io430.h"
#include "flash.h"

#define _CPU_ 5

void flashWb(unsigned char * ptr, unsigned char byte)
{
    FCTL3 = 0xA500;
    FCTL1 = 0xA540;
    * ptr = byte;
    FCTL1 = 0xA500;
    FCTL3 = 0xA510;
}

void flashWbSegA(unsigned char * ptr, unsigned char byte)
{
    FCTL3 = 0xA540;
    FCTL1 = 0xA540;
    * ptr = byte;
    FCTL1 = 0xA500;
    FCTL3 = 0xA550;
}

void flashWW(unsigned int * ptr, unsigned int byte)
{
    FCTL3 = 0xA500;
    FCTL1 = 0xA540;
    * ptr = byte;
    FCTL1 = 0xA500;
    FCTL3 = 0xA510;
}

void flashClr(unsigned int * ptr)
{
  FCTL2 = FWKEY + FSSEL_1 + 0x3F;
  FCTL3 = 0xA500;
  FCTL1 = 0xA502;
  * ptr = 0;
  FCTL1 = 0xA500;
  FCTL3 = 0xA510;
}

void flashWriteBuff(unsigned char * addr, unsigned char * buff, unsigned char num)
{
  int i;

  for(i=0;i<num;i++)
  {
    flashWb(addr,buff[i]);
    addr++;
  }
}
