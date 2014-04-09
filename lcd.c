#include <intrinsics.h>
#include "io430.h"
#include "lcd.h"

//Define a porta onde o LCD será conectado
#define LCD_PSEL P1SEL
#define LCD_PDIR P1DIR
#define LCD_POUT P1OUT
#define LCD_PIN  P1IN

//Define porta do backlight
#define LCD_BL_PSEL P4SEL
#define LCD_BL_PDIR P4DIR
#define LCD_BL_POUT P4OUT

//Bits de controle
#define LCD_E BIT0
#define LCD_CD BIT1
#define LCD_RW BIT2
#define LCD_CONTR BIT3
#define LCD_BL BIT7

//Bits de dados
#define LCD_D0 BIT4
#define LCD_D1 BIT5
#define LCD_D2 BIT6
#define LCD_D3 BIT7

#define LCD_LINES 2
#define LCD_CHARS_PER_LINE  16

struct lcdDisplay {
  unsigned char line1[16];
  unsigned char line2[16];
};

static struct lcdDisplay lcd;
static struct lcdDisplay lcd2;
static unsigned char modeSearchTicks = 0;
static int initTicks = 0;
unsigned char lcdTicks = 0;

//Funções de escopo local
static void lcdDelayDado(void);
static void lcdDelayInst(void);
static void lcdInitIO(void);
static void lcdInstructionWrite(unsigned char);
static void lcdDataWrite(unsigned char);
static void lcdClear(void);
static void lcdInitTimer(unsigned int, unsigned int);

static int rtTicks(void)
{
  extern int ticks;

  return ticks;
}

/*!  brief Espera um tempo até que o LCD possa ser configurado */
void lcdWait(void)
{
  int i,j;

  for (i=0;i<30000;i++)
      for(j=0;j<100;j++);
}

/*!  \brief Delay entre o envio de dados */
static void lcdDelayDado(void)
{
  unsigned int i;

  for (i = 800; i > 0; i--);
}

/*! \brief Delay entre instruções
*/
static void lcdDelayInst(void)
{
  unsigned int i;

  for (i = 1200; i > 0; i--);
}

/*! \brief Inicia a porta responsável por comunicar com o LCD */
void lcdInitIO(void)
{
  //Seleciona os pinos de controle como saída e configura o seu valor
  LCD_PSEL &= ~(LCD_E + LCD_CD + LCD_RW);
  LCD_PDIR |= (LCD_E + LCD_CD + LCD_RW);
  LCD_POUT &= ~(LCD_CD + LCD_RW);
  LCD_POUT &= ~(LCD_E);

  //Seleciona os pinos de dados como saída
  LCD_PSEL &= ~(LCD_D0 + LCD_D1 + LCD_D2 + LCD_D3);
  LCD_PDIR |= (LCD_D0 + LCD_D1 + LCD_D2 + LCD_D3);

  //Acende o backlight
  LCD_BL_PSEL &= ~(LCD_BL);
  LCD_BL_PDIR |= LCD_BL;
  LCD_BL_POUT |= LCD_BL;

  // configura o pino do contraste
  LCD_PDIR |= LCD_CONTR;
}

/*!  \brief Configura o PWM responsável pelo ajuste do contraste */
static void lcdInitTimer(unsigned int period, unsigned int periodOn)
{
  /*
  //Seleciona o pino como PWM e saída
  LCD_PDIR |= LCD_CONTR;
  LCD_PSEL |= LCD_CONTR;

  //Configura o período total e o período ligado
  TACCR0 = period;
  TACCTL2 = OUTMOD_7;
  TACCR2 = periodOn;

  //Configura o timer
  TACTL = TASSEL_2 + MC_1;
  */
}

/*! \brief Envia comando para o controlador do LCD
*/
static void lcdInstructionWrite(unsigned char inst)
{
   // reseta o bit !C/D para indicar dado
   LCD_POUT &= ~LCD_CD;

   // coloca o primeiro nibble no barramento
   LCD_POUT = (inst & 0xF0) + (LCD_POUT & 0x0F);

   // pulsa o bit CS (ENABLE)
   LCD_POUT |= LCD_E;
   LCD_POUT &= ~LCD_E;

   //Coloca o segundo nibble no barramento
   LCD_POUT = ((inst & 0x0F) << 4) + (LCD_POUT & 0x0F);

   // pulsa o bit CS (ENABLE)
   LCD_POUT |= LCD_E;
   LCD_POUT &= ~LCD_E;

   lcdDelayInst();
}

/*! \brief Escreve uma caracter na posição atual do LCD
*/
static void lcdDataWrite(unsigned char data)
{
   // seta o bit !C/D para indicar dado
   LCD_POUT |= LCD_CD;

   // coloca o dado no barramento
   LCD_POUT = (data & 0xF0) + (LCD_POUT & 0x0F);

   // pulsa o bit CS (ENABLE)
   LCD_POUT |= LCD_E;
   LCD_POUT &= ~LCD_E;

   //Coloca o segundo nibble no barramento
   LCD_POUT = ((data & 0x0F) << 4) + (LCD_POUT & 0x0F);

   // pulsa o bit CS (ENABLE)
   LCD_POUT |= LCD_E;
   LCD_POUT &= ~LCD_E;

   lcdDelayDado();
}

/*! \brief Envia comando para o controlador do LCD
*/
static void lcdInstructionWriteNoDelay(unsigned char inst)
{
   // reseta o bit !C/D para indicar dado
   LCD_POUT &= ~LCD_CD;

   // coloca o primeiro nibble no barramento
   LCD_POUT = (inst & 0xF0) + (LCD_POUT & 0x0F);

   // pulsa o bit CS (ENABLE)
   LCD_POUT |= LCD_E;
   LCD_POUT &= ~LCD_E;

   //Coloca o segundo nibble no barramento
   LCD_POUT = ((inst & 0x0F) << 4) + (LCD_POUT & 0x0F);

   // pulsa o bit CS (ENABLE)
   LCD_POUT |= LCD_E;
   LCD_POUT &= ~LCD_E;
}

/*! \brief Escreve uma caracter na posição atual do LCD
*/
static void lcdDataWriteNoDelay(unsigned char data)
{
   // seta o bit !C/D para indicar dado
   LCD_POUT |= LCD_CD;

   // coloca o dado no barramento
   LCD_POUT = (data & 0xF0) + (LCD_POUT & 0x0F);

   // pulsa o bit CS (ENABLE)
   LCD_POUT |= LCD_E;
   LCD_POUT &= ~LCD_E;

   //Coloca o segundo nibble no barramento
   LCD_POUT = ((data & 0x0F) << 4) + (LCD_POUT & 0x0F);

   // pulsa o bit CS (ENABLE)
   LCD_POUT |= LCD_E;
   LCD_POUT &= ~LCD_E;
}

/*!  \brief Limpa o display */
static void lcdClear(void)
{
  lcdInstructionWrite(0x01);
  lcdDelayDado();
}

/*!  \brief Rotina de iniciação do display */
void lcdInit(void)
{
   //Inicia o hw
   lcdInitIO();

   //Inicia o pwm do contraste
   lcdInitTimer(PWM_PERIOD, PWM_PERIOD_ON);

   //Delay até que a tensão de alimentação se normalize
   lcdDelayInst();
   lcdDelayInst();
   lcdDelayInst();
   lcdDelayInst();

   lcdClear();

   // 8 bits, 2 linhas e 5x8 dots
   //lcdInstructionWrite(0x38);
   lcdInstructionWrite(0x28);
   lcdDelayInst();

   // 8 bits, 2 linhas e 5x8 dots
   //lcdInstructionWrite(0x38);
   lcdInstructionWrite(0x28);
   lcdDelayInst();

   // Sentido de movimentação do vetor
   lcdInstructionWrite(0x06);
   lcdDelayInst();

   // Liga display e cursor
   lcdInstructionWrite(0x0C);
   lcdDelayInst();
}

/*!  \brief Armazena em memória o valor a ser colocado na tela
 *   \param dataMsg Mensagem a ser gravada
 *   \param row Posição da linha
 *   \param col Posição da coluna
 */
void lcdPutString(unsigned char * dataMsg,unsigned char row, unsigned char col)
{
  unsigned char countMsg;
  unsigned char countLcd;

  if(row < LCD_LINES)
  {
    countMsg = 0;
    countLcd = col;

    while((dataMsg[countMsg] != 0x00) && (countLcd < (LCD_CHARS_PER_LINE)))
    {
      if(row == 0)
      {
        lcd.line1[countLcd] = dataMsg[countMsg];
      }
      else
      {
        lcd.line2[countLcd] = dataMsg[countMsg];
      }
      countMsg++;
      countLcd++;
    }
  }

  lcdUpdate();
}

/*!  \brief Armazena em memória o valor a ser colocado na tela de procura
 *   \param dataMsg Mensagem a ser gravada
 *   \param row Posição da linha
 *   \param col Posição da coluna
 *   \param Tempo sem segundos que a mensagem deve aparecer
 */
void lcdPutStringSearch(unsigned char * dataMsg, unsigned char row, unsigned char col, unsigned char timeout)
{
    unsigned char countMsg;
    unsigned char countLcd;

    //Diz o timeout para essa mensagem
    modeSearchTicks = timeout * 12;
    initTicks = rtTicks();
    
    if(row < LCD_LINES)
    {
      countMsg = 0;
      countLcd = col;

      while((dataMsg[countMsg] != 0x00) && (countLcd < (LCD_CHARS_PER_LINE)))
      {
        if(row == 0)
        {
          lcd2.line1[countLcd] = dataMsg[countMsg];
        }
        else
        {
          lcd2.line2[countLcd] = dataMsg[countMsg];
        }
        countMsg++;
        countLcd++;
      }
    }
    
    lcdUpdate();
}

/*!  \brief Coloca na tela a mensagem */
void lcdUpdate(void)
{
   char i;
   struct lcdDisplay * dsp;
   int endTicks;
   
   dsp = &lcd;
   
   if(modeSearchTicks > 0)
   {
     //Existe uma mensagem de modo de busca para colocar na tela
     endTicks = rtTicks();
     if((endTicks - initTicks) >= modeSearchTicks)
     {
       //Timeout do modo de buscas expirou
       modeSearchTicks = 0;
     }
     else
     {
       //Apresenta a mensagem do modo de busca do wireless
       dsp = &lcd2;
     }
   }

   lcdInstructionWrite(0x80);
   for(i = 0; i < 16; i++)
   {
      lcdDataWrite(dsp->line1[i]);
   }

   lcdInstructionWrite(0xC0);
   for(i = 0; i < 16; i++)
   {
      lcdDataWrite(dsp->line2[i]);
   }
}

/*!  \brief Liga o backlight */
void lcdBacklightOn(void)
{
  LCD_BL_POUT |= LCD_BL;
}

/*!  \brief Desliga o backlight */
void lcdBacklightOff(void)
{
  LCD_BL_POUT &= ~LCD_BL;
}

/*!  \brief Atualiza lcd no fundo utilizando o timer para controlar o tempo
 *   \return Retorna 1 se o procedimento de atualização foi finalizado
 */
unsigned char lcdLowUpdate(void)
{
   static unsigned char state = 0;
   static unsigned char count = 0;
   struct lcdDisplay * dsp;
   int endTicks;
   
   dsp = &lcd;
   
   if(modeSearchTicks > 0)
   {
     //Existe uma mensagem de modo de busca para colocar na tela
     endTicks = rtTicks();
     if((endTicks - initTicks) >= modeSearchTicks)
     {
       //Timeout do modo de buscas expirou
       modeSearchTicks = 0;
     }
     else
     {
       //Apresenta a mensagem do modo de busca do wireless
       dsp = &lcd2;
     }
   }
   
   switch(state)
   {
     case 0:
       lcdInstructionWriteNoDelay(0x80);
       state = 1;
       lcdTicks = 0;
     break;

     case 1:
      if(lcdTicks >= 2)
      {
           state = 2;
      }
     break;

     case 2:
       lcdDataWriteNoDelay(dsp->line1[count]);
       count++;
       state = 3;
       lcdTicks = 0;

       if(count == 16)
       {
         count = 0;
         state = 4;
       }
     break;

     case 3:
       if(lcdTicks >= 2)
       {
           state = 2;
       }
     break;

     case 4:
       if(lcdTicks >= 2)
       {
           state = 5;
       }
     break;

     case 5:
       lcdInstructionWriteNoDelay(0xC0);
       state = 6;
       lcdTicks = 0;
     break;

     case 6:
       if(lcdTicks >= 2)
       {
           state = 7;
       }
     break;

     case 7:
       lcdDataWriteNoDelay(dsp->line2[count]);
       count++;
       state = 8;
       lcdTicks = 0;

       if(count == 16)
       {
         count = 0;
         state = 0;
         return 1;
       }
     break;

     case 8:
       if(lcdTicks >= 2)
       {
           state = 7;
       }
     break;
   }
  return 0;
}

unsigned char lcdGetChar(unsigned char num)
{
  return (lcd.line2[num]);
}

void lcdSetChar(unsigned char val, unsigned char num, unsigned char col)
{
  if(col)
  {
    lcd.line2[num] = val;
  }
  else
  {
    lcd.line1[num] = val;
  }
}

void lcdContrSet(void)
{
    LCD_POUT |= LCD_CONTR;
}

void lcdContrClear(void)
{
    LCD_POUT &= ~(LCD_CONTR);
}