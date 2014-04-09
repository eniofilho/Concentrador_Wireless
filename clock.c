#include <intrinsics.h>
#include "io430.h"
#include "clock.h"

extern int ticks;

#define CLK_PSEL    P5SEL
#define CLK_PDIR    P5DIR
#define SMCLK_BIT   BIT5
#define MCLK_BIT    BIT4

void clockInit(void)
{
  // configura o DCO para 16MHz
  __bis_SR_register(OSCOFF);

  if(CALBC1_16MHZ != 0xFF)
  {
    //Info flash está intacta
    DCOCTL = 0x00;
    BCSCTL1= CALBC1_16MHZ;
    DCOCTL = CALDCO_16MHZ;
  }
  else
  {
    //Info flash foi apagada, chuta valores próximos ao ideal
    BCSCTL1 = 0x8F;
    DCOCTL = 0x7F;
  }

  // configura todos os clocks a partir do DCO
  BCSCTL2 = 0;

  //Configura VLO
  BCSCTL3 = LFXT1S_2;

#if 0
  //Clock no pino 48 e 49
  CLK_PSEL |= (MCLK_BIT + SMCLK_BIT);
  CLK_PDIR |= (MCLK_BIT + SMCLK_BIT);
#endif
}

clock_time_t clock_time(void)
{
  return ticks;
}
