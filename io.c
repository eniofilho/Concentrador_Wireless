#include <intrinsics.h>
#include "io430.h"
#include "io.h"

//Define endereço de portas e bits onde os relés estão conectados
#define RELE_PSEL P2SEL
#define RELE_PDIR P2DIR
#define RELE_POUT P2OUT
#define RELE_1 BIT0
#define RELE_2 BIT3
#define RELE_3 BIT7
#define RELE_4 BIT1

unsigned char releChanged = 1;
unsigned char releState[4] = {'0','0','0','0'};

/*!  \brief Configura o hardware para esse módulo */
void ioInit(void)
{
  //Inicia a configuração dos pinos
  RELE_PSEL &= ~(RELE_1 + RELE_2 + RELE_4 + RELE_3);
  RELE_PDIR |= (RELE_1 + RELE_2 + RELE_4 + RELE_3);
  RELE_POUT &= ~(RELE_1 + RELE_2 + RELE_4 + RELE_3);
}

/*!  \brief Aciona o alarme
 *   \param num Número do acionador
 */
void ioReleSet(unsigned char num)
{
  releChanged = 1;

  switch (num)
  {
  case 1:
    RELE_POUT |= RELE_1;
    releState[0] = '1';
    break;

  case 2:
    RELE_POUT |= RELE_2;
    releState[1] = '2';
    break;

  case 3:
    RELE_POUT |= RELE_3;
    releState[2] = '3';
    break;

  case 4:
    RELE_POUT |= RELE_4;
    releState[3] = '4';
    break;

  default:
    //Não houve modificação no relé
    releChanged = 0;
    break;
  }
}

/*!  \brief Desaciona o alarme
 *   \param num Número do acionador
 */
void ioReleClear(unsigned char num)
{
  releChanged = 1;

  switch (num)
  {
  case 1:
    RELE_POUT &= ~RELE_1;
    releState[0] = '0';
    break;

  case 2:
    RELE_POUT &= ~RELE_2;
    releState[1] = '0';
    break;

  case 3:
    RELE_POUT &= ~RELE_3;
    releState[2] = '0';
    break;

  case 4:
    RELE_POUT &= ~RELE_4;
    releState[3] = '0';
    break;

  default:
    releChanged = 0;
    break;
  }
}

/*!  \brief Indica se houve mudança no status do rele */
unsigned char ioReleChanged(void)
{
    return releChanged;
}

/*!  \brief Retorna o status do rele
 *   \param Número do rele
 */
unsigned char ioReleStatus(unsigned char num)
{
    return releState[num];
}