#include <intrinsics.h>
#include "io430.h"
#include "serial.h"
#include "clock.h"
#include "wtd.h"
#include "lcd.h"
#include "utils.h"

//Defines para os bits das seriais
#define SERIAL1   BIT0
#define SERIAL2   BIT1
#define SERIAL3   BIT2
#define SERIAL4   BIT3
#define SERIAL5   BIT4
#define SERIAL6   BIT5
#define SERIAL7   BIT6
#define SERIAL8   BIT7
#define SERIAL9   BIT5
#define SERIAL10  BIT6

//Botão do concentrador
#define CONC_BT_PSEL  P5SEL
#define CONC_BT_PDIR  P5DIR
#define CONC_BT_PIN   P5IN
#define CONC_BT_PREN  P5REN
#define CONC_BT_BIT   BIT5

//Debounce para o botão do concentrador
#define DEBOUNCE_BT 200

/*!  \brief Taxa da comunicação serial com os sensores */
#define BAUD 1200

/*!  \brief Número de amostras por bit de dados */
#define SAMPLE_BIT 10

/*!  \brief Número de amostras para o primeiro bit amostrado */
#define SAMPLE_START (SAMPLE_BIT + SAMPLE_BIT/2 -2)

/*!  \brief Frequencia de amostragem dos dados seriais */
#define FS (BAUD*SAMPLE_BIT)

/*!  \brief Valor armazenado no timer para a fs desejada */
#define TIMER_CNTR CLK/FS

/*!  \brief Tamanho do identificador dos sensores */
#define DATA_SIZE 64
//#define DATA_SIZE 72 //I ADDED THIS TO COUNT BATTERY DATA (+ 8 bits)

/*!  \brief Estado 0: Espera pelo inicio da transmissão */
#define IDLE 0
/*!  \brief Estado 1: Espera até o momento da primeira amostragem */
#define START_BIT 1
/*!  \brief Estado 2: Amostra os dados */
#define DATA 2
/*!  \brief Espera o momento até a amostragem do próximo bit */
#define DATA_WAIT 3
/*!  \brief Verifica o fim da comunicação */
#define STOP_BIT 4
/*!  \brief */
#define DATA_WAIT_RESYNC_1 5
/*!  \brief */
#define DATA_WAIT_RESYNC_2 6
/*!  \brief */
#define DATA_WAIT_RESYNC_3 7

#pragma pack(1)
typedef struct
{
  unsigned long identificador;
  unsigned char tipo;
  //unsigned int batt; //i added this
  unsigned int valor;
  unsigned char estado;
  unsigned int count;
  unsigned char dataTemp[8];
  unsigned char rxBits;
  unsigned char async;
}SERIAL_ST;

//Protótipo das funções de escopo local
static void serialInitIO(void);
static void timerInit(void);
static unsigned char chkCalc(unsigned char *,unsigned char);

/*!  \brief System ticks for network timeout */
int ticks = 0;

/*!  \brief Variável auxiliar para a contagem de ticks */
static int ticksCount = 0;

/*!  \brief Guarda a amostra atual da interface serial */
static unsigned char dadoSerial[10];

/*!  \brief Armazena as informações de cada interface serial */
SERIAL_ST serial[10];

/*!  \brief Armazena o pino de entrada do concentrador */
unsigned char concBt = 0;

/*!  \brief Utilizado para o debounce do pino do concentrador */
unsigned char concBtCount = 0;

/*!  \brief Envia dado recebido asincronamente */
extern unsigned char countBaud;
extern unsigned int countLcdInit;
extern unsigned int countWireless;

/*!  \brief Inicia o hardware responsável pela comunicação serial */
static void serialInitIO(void)
{
  //Define portas como IO e entrada
  //Portas 1 a 8
  P6SEL &= ~(SERIAL1 + SERIAL2 + SERIAL3 + SERIAL4 + SERIAL5 + SERIAL6 + SERIAL7 + SERIAL8);
  P6DIR &= ~(SERIAL1 + SERIAL2 + SERIAL3 + SERIAL4 + SERIAL5 + SERIAL6 + SERIAL7 + SERIAL8);
  P6OUT |= (SERIAL1 + SERIAL2 + SERIAL3 + SERIAL4 + SERIAL5 + SERIAL6 + SERIAL7 + SERIAL8);
  P6REN |= (SERIAL1 + SERIAL2 + SERIAL3 + SERIAL4 + SERIAL5 + SERIAL6 + SERIAL7 + SERIAL8);
  
  //Portas 9 e 10
  P2SEL &= ~(SERIAL9 + SERIAL10);
  P2DIR &= ~(SERIAL9 + SERIAL10);
  P2OUT |= (SERIAL9 + SERIAL10);
  P2REN |= (SERIAL9 + SERIAL10);

  //Botão do concentrador
  CONC_BT_PREN |= CONC_BT_BIT;
  CONC_BT_PSEL &= ~CONC_BT_BIT;
  CONC_BT_PDIR &= ~CONC_BT_BIT;

  //Teste
  P4SEL &= ~(BIT0+BIT1);
  P4DIR |= (BIT0+BIT1);
  P4OUT |= (BIT0+BIT1);
}

/*!  \brief Inicia o timer responsável pela serial */
static void timerInit(void)
{
  //Habilita interrupção
  TACCTL0 = CCIE;

  //Período do timer
  TACCR0 = TIMER_CNTR;

  TACTL = TASSEL_2 + MC_1;
}

/*!  \brief Inicia o hw e as variáveis responsáveis pela comunicação serial */
void serialInit(void)
{
  //Inicia as portas
  serialInitIO();

  //Inicia o timer responsável pelas leituras
  timerInit();

  //Limpa a flag da serial
  sensorClear();
}

// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{
  static unsigned int lcdContrTimer = 0;
  unsigned char i;

  if (++lcdContrTimer == PWM_PERIOD_ON)
  {
      lcdContrClear();
  }
  else if (lcdContrTimer >= PWM_PERIOD)
  {
      lcdContrSet();
      lcdContrTimer = 0;
  }

  wtdFlag = 0xFF;

  lcdTicks++;
  countLcdInit++;

  if(ticksCount >= 1000)
  {
    //Incremente em cada 1/12 de segundo
    ticks++;
    countWireless++;
    ticksCount = 0;
  }
  else
  {
    ticksCount++;
  }

  //Verifica o status do botão do concentrador
  if(concBt)
  {
    if(CONC_BT_PIN & CONC_BT_BIT)
    {
      concBtCount++;
    }
    else
    {
      concBtCount = 0;
    }

    if(concBtCount > DEBOUNCE_BT)
    {
      concBt = 0xFF;
    }
  }
  else
  {
    if(!(CONC_BT_PIN & CONC_BT_BIT))
    {
      concBtCount++;
    }
    else
    {
      concBtCount = 0;
    }

    if(concBtCount > DEBOUNCE_BT)
    {
      concBt = 0;
    }
  }

  dadoSerial[0] = P6IN & SERIAL1;
  dadoSerial[1] = P6IN & SERIAL2;
  dadoSerial[2] = P6IN & SERIAL3;
  dadoSerial[3] = P6IN & SERIAL4;
  dadoSerial[4] = P6IN & SERIAL5;
  dadoSerial[5] = P6IN & SERIAL6;
  dadoSerial[6] = P6IN & SERIAL7;
  dadoSerial[7] = P6IN & SERIAL8;
  dadoSerial[8] = P2IN & SERIAL9;
  dadoSerial[9] = P2IN & SERIAL10;

  for(i=0;i<10;i++)
  {
    switch(serial[i].estado)
    {
      case IDLE:
        if(!dadoSerial[i])
        {
          serial[i].estado = START_BIT;
          serial[i].count = 0;
        }
        else
        {
          serial[i].count++;

          if(serial[i].count > (FS/2))
          {
            //Muito tempo sem receber informação do sensor
            serial[i].identificador = 0xFFFFFFFF;
            serial[i].tipo = 0xFF;
            //serial[i].batt = 0xFF; //i added this
            serial[i].valor = 0xFFFF;
          }
        }
      break;

      case START_BIT:
        if(serial[i].count >= (SAMPLE_START))
        {
          unsigned char j;

          serial[i].count = 0;
          for(j=0;j<8;j++)
          {
            serial[i].dataTemp[j] = 0;
          }
          serial[i].estado = DATA;
          serial[i].rxBits = 0;
        }
        else
        {
          serial[i].count++;
        }
      break;

      case DATA:
        if(i == 0)
        {
            P4OUT ^= BIT1;
        }

        if(dadoSerial[i])
        {
          serial[i].dataTemp[serial[i].rxBits >> 3] |= 0x01;
        }

        if(serial[i].rxBits < DATA_SIZE - 1)
        {
          serial[i].rxBits++;
          if(serial[i].rxBits & 0x0F)
          {
            //Desloca quando não é o bit menos significativo do byte
            serial[i].dataTemp[serial[i].rxBits >> 3] <<= 1;
          }

          if((serial[i].rxBits & 0x07) == 0x00)
          {
            serial[i].estado = DATA_WAIT_RESYNC_1;
            serial[i].count = 0;
          }
          else
          {
            serial[i].estado = DATA_WAIT;
            serial[i].count = 0;
          }
        }
        else
        {
          serial[i].rxBits++;
          serial[i].estado = DATA_WAIT;
          serial[i].count = 0;
        }
      break;

      case DATA_WAIT_RESYNC_1:
        if(dadoSerial[i])
        {
            serial[i].estado = DATA_WAIT_RESYNC_2;
            serial[i].count = 0;
        }
        else
        {
            serial[i].count++;

            if(serial[i].count > (3*SAMPLE_BIT))
            {
                serial[i].count = 0;
                serial[i].estado = IDLE;
            }
        }
      break;

      case DATA_WAIT_RESYNC_2:
        if(!dadoSerial[i])
        {
            serial[i].estado = DATA_WAIT_RESYNC_3;
            serial[i].count = 0;
        }
        else
        {
            serial[i].count++;

            if(serial[i].count > (3*SAMPLE_BIT))
            {
                serial[i].count = 0;
                serial[i].estado = IDLE;
            }
        }
      break;

      case DATA_WAIT_RESYNC_3:
        if(serial[i].count >= (SAMPLE_START))
        {
          serial[i].count = 0;
          serial[i].estado = DATA;
        }
        else
        {
          serial[i].count++;
        }
      break;

      case DATA_WAIT:
        if(serial[i].count >= (SAMPLE_BIT - 2))
        {
          //Recebe os 64 bits (identificador + tipo + valor + CRC) 
          if(serial[i].rxBits == DATA_SIZE)
          {
            serial[i].estado = STOP_BIT;
            serial[i].count = 0;
          }
          else
          {
            serial[i].estado = DATA;
          }
        }
        else
        {
          serial[i].count++;
        }
      break;

      case STOP_BIT:
        if(dadoSerial[i])
        {
          serial[i].count++;
        }
        else
        {
          serial[i].count = 0;
        }

        //Espera por um tempo considerável sem dados para o sincronismo
        if((serial[i].count > SAMPLE_BIT*20))
        {
          if(serial[i].async == 0)
          {
              if(chkCalc(&(serial[i].dataTemp[0]),7) == serial[i].dataTemp[7])
              {
                unsigned int temp;

                serial[i].identificador = ((unsigned long)serial[i].dataTemp[0]) << 24;
                serial[i].identificador += ((unsigned long)serial[i].dataTemp[1]) << 16;
                serial[i].identificador += ((unsigned long)serial[i].dataTemp[2]) << 8;
                serial[i].identificador += ((unsigned long)serial[i].dataTemp[3]);

                serial[i].tipo = serial[i].dataTemp[4];

                temp = ((unsigned int)serial[i].dataTemp[5]) << 8;
                temp += ((unsigned int)serial[i].dataTemp[6]);

                if((serial[i].valor == 0xFFFF) && (temp == 0x0000) && (serial[i].tipo == 0x01))
                {
                    //Envia o dado assincronamente
                    countBaud = 0xFD;
                    serial[i].async = 1;
                }

                serial[i].valor = temp;
              }
              else
              {
                if(serial[i].valor != 0xFFFE)
                {
                    //Erro de CRC
                    serial[i].identificador = 0xFFFFFFFE;
                    //serial[i].batt = 0xFE; //i added this
                    serial[i].tipo = 0xFE;
                    serial[i].valor = 0xFFFE;

                    //Envia dado assincronamente
                    countBaud = 0xFD;
                }
              }
          }
          serial[i].estado = IDLE;
          serial[i].count = 0;
        }
      break;

      default:
        serial[i].estado = IDLE;
        serial[i].count = 0;
      break;
    }
  }
}

/*!  \brief Calcula o checksum dos dados do identificador
 *   \param pmsg Ponteiro para os dados do identificador
 *   \param msgSize Quantidade de bytes do identificador
 */
static unsigned char chkCalc(unsigned char * pmsg,unsigned char msgSize)
{
  unsigned int chk;
  unsigned char i;

  chk = 0;
  for(i=0;i<msgSize;i++)
  {
    chk += pmsg[i];
  }

  return (unsigned char)chk;
}

/*!  \brief Retorna o status do botão do concentrador */
unsigned char sensorGetConcStatus(void)
{
  if(concBt)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

/*!  \brief Obtem o id de uma determinado sensor*/
void sensorGetId(unsigned char num, void * data)
{
  unsigned char * dataCh = (unsigned char *)data;

  if(serial[num].identificador == 0xFFFFFFFE)
  {
      dataCh[0] = '?';
      dataCh[1] = '?';
      dataCh[2] = '?';
      dataCh[3] = '?';
      dataCh[4] = '?';
      dataCh[5] = '?';
      dataCh[6] = '?';
      dataCh[7] = '?';
  }
  else
  {
      dataCh[0] = hexToChar((serial[num].identificador >> 28) & 0x0F);
      dataCh[1] = hexToChar((serial[num].identificador >> 24) & 0x0F);
      dataCh[2] = hexToChar((serial[num].identificador >> 20) & 0x0F);
      dataCh[3] = hexToChar((serial[num].identificador >> 16) & 0x0F);
      dataCh[4] = hexToChar((serial[num].identificador >> 12) & 0x0F);
      dataCh[5] = hexToChar((serial[num].identificador >> 8) & 0x0F);
      dataCh[6] = hexToChar((serial[num].identificador >> 4) & 0x0F);
      dataCh[7] = hexToChar((serial[num].identificador) & 0x0F);
  }
}

/*!  \brief Obtem a string de tipo de um determinado sensor */
void sensorGetTipo(unsigned char num, void * data)
{
  unsigned char * dataCh = (unsigned char *) data;

  if(serial[num].tipo == 0xFE)
  {
      dataCh[0] = '?';
      dataCh[1] = '?';
  }
  else
  {
      dataCh[0] = hexToChar((serial[num].tipo >> 4) & 0x0F);
      dataCh[1] = hexToChar((serial[num].tipo) & 0x0F);
  }
}

/*!  \brief Obter a string de valor de um determinado sensor */
void sensorGetValor(unsigned char num, void * data)
{
  unsigned char * dataCh = (unsigned char *) data;

  if(serial[num].valor == 0xFFFE)
  {
      dataCh[0] = '?';
      dataCh[1] = '?';
      dataCh[2] = '?';
      dataCh[3] = '?';
  }
  else
  {
      dataCh[0] = hexToChar((serial[num].valor >> 12) & 0x0F);
      dataCh[1] = hexToChar((serial[num].valor >> 8) & 0x0F);
      dataCh[2] = hexToChar((serial[num].valor >> 4) & 0x0F);
      dataCh[3] = hexToChar((serial[num].valor) & 0x0F);
  }
}

////I added this
///*!  \brief Obter a string de bateria de um determinado sensor */
//void sensorGetBatt(unsigned char num, void * data)
//{
//  unsigned char * dataCh = (unsigned char *) data;
//
//  if(serial[num].batt == 0xFE)
//  {
//      dataCh[0] = '?';
//      dataCh[1] = '?';
//  }
//  else
//  {
//      dataCh[0] = hexToChar((serial[num].tipo >> 4) & 0x0F);
//      dataCh[1] = hexToChar((serial[num].tipo) & 0x0F);
//  }
//}

/*!  \brief Função parecida a sensorGetValor, porém retorna o status dos sensores sem gravar no buffer */
unsigned char sensorIsSet(unsigned char num)
{
  if(serial[num].valor == 0xFFFE)
  {
    return '?';
  }
  else if(serial[num].valor == 0x0000)
  {
    return '*';
  }
  else
  {
    if(serial[num].identificador == 0xFFFFFFFF)
    {
      return '-';
    }
    else
    {
      return hexToChar(num + 1);
    }
  }
}

void sensorSetDataSent(void)
{
    unsigned char i;

    for(i=0;i<10;i++)
    {
        if(serial[i].async == 1)
        {
            serial[i].async = 2;
        }
    }
}

/*!  \brief Diz que os dados do sensor já foram enviados */
void sensorClear(void)
{
    unsigned char i;

    for(i=0;i<10;i++)
    {
        if(serial[i].async == 2)
        {
           serial[i].async = 0;
        }
    }
}