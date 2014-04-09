#include <intrinsics.h>
#include "io430.h"
#include "wtd.h"

unsigned char wtdFlag = 0;

/*!  \brief Desabilita o watchdog */
void wtdStop(void)
{
  WDTCTL = WDTPW + WDTHOLD;
}

/*! \brief Kick the dog */
void wtdClear(void)
{
  WDTCTL = WDTPW + WDTCNTCL + WDTSSEL;
}

/*!  \brief Verifica se watchdog foi corretamente setado na interrupção */
void wtdTest(void)
{
  if(wtdFlag == 0xFF)
  {
    wtdClear();
    wtdFlag = 0;
  }
}