/*! \file uart.h
 *  \brief interface publica para o objeto uart.
 */
#ifndef __UART_H__
#define __UART_H__

//Porta de comunicação com o módulo de rádio
#define UART_PSEL	P3SEL
#define UART_PDIR	P3DIR
#define UART_POUT	P3OUT
#define UART_PIN	P3IN

//Bits utilizados na comunicação
#define UART_TX	BIT6
#define UART_RX	BIT7

#define UART_TX_BUFFER_SIZE 32
#define UART_RX_BUFFER_SIZE 256

typedef enum
{
   UART_SPEED_2400 = 0,
   UART_SPEED_4800,
   UART_SPEED_9600,
   UART_SPEED_19200,
   UART_SPEED_38400,
   UART_SPEED_57600,
   UART_SPEED_115200
} UART_SPEED;

typedef enum
{
   UART_BITS_8 = 0,
   UART_BITS_7
} UART_BITS;

typedef enum
{
   UART_PARITY_NONE = 0,
   UART_PARITY_EVEN,
   UART_PARITY_ODD
} UART_PARITY;

typedef enum
{
   UART_STOP_BITS_1 = 0,
   UART_STOP_BITS_2
} UART_STOP_BITS;

typedef enum
{
   UART_STATE_OFF = 0,
   UART_STATE_ON,
   UART_STATE_NOT_INITIALIZED
} UART_STATE;

typedef struct UART_STRUCT
{
   void (* init)              (void * puart);
   void (* config)            (void * puart, UART_SPEED speed, UART_BITS bits, UART_PARITY parity, UART_STOP_BITS stopBits);
   void (* start)             (void * puart);
   void (* putBuffTx)         (void * puart, unsigned char data);
   char (* getBuffTx)         (void * puart, unsigned char * data);
   void (* putBuffRx)         (void * puart, unsigned char data);
   char (* getBuffRx)         (void * puart, unsigned char * data);
   void (* stop)              (void * puart);
   void (* reset)             (void * puart);

   UART_STATE     state;
   UART_SPEED     speed;
   UART_BITS      bits;
   UART_PARITY    parity;
   UART_STOP_BITS stopBits;
   char           txBuffer[UART_TX_BUFFER_SIZE];
   unsigned int   txPtrIn;
   unsigned int   txPtrOut;
   char           rxBuffer[UART_RX_BUFFER_SIZE];
   unsigned int   rxPtrIn;
   unsigned int   rxPtrOut;
} UART;

extern UART uart1;

#endif