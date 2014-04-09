/*! \file uart.h
 *  \brief interface publica para o objeto uart.
 */
#ifndef __W_OP_MAQ_H__
#define __W_OP_MAQ_H__

#include "uart.h"

#define SZ_SERIAL_BUFFER        300

typedef enum
{
   MODE_RX = 0,
   MODE_SEARCH,
   MODE_INV
} W_MODE;

typedef enum
{
   RX_IDLE = 0,
   RX_MODE_INV,
   RX_MODE_SEARCH,
   RX_MODE_RX,
   RX_WAIT_SEND,
   RX_WAIT_RESP,
   RX_SENSOR_LIST,
} RX_STATE;

typedef enum
{
  W_MSG_NONE = 0,
  W_MSG_INV,
  W_MSG_RX,
  W_MSG_RESP,
  W_MSG_SENSOR_LIST,
} W_MSG;

#pragma pack(1)
typedef struct W_OP_MAQ_STRUCT
{
   void (* init)                                (void *);
   void (* setMode)                             (void *, W_MODE);
   void (* writeSens)                           (void *, unsigned char *);
   void (* eraseSens)                           (void *, unsigned char *);
   void (* listSens)                            (void *);
   void (* changeCh)                            (void *, unsigned char);
   void (* readCh)                              (void *);
   void (* cleanRx)                             (void *);
   unsigned char (* wirelessMsgReceived)        (void *);
   void (* rxData)                              (void *);
   
   unsigned int ptSerial;
   RX_STATE     state;
   UART *       uart;
   unsigned char serialBuffer[SZ_SERIAL_BUFFER];
   W_MSG msgType;
   
} W_OP_MAQ;

extern W_OP_MAQ wOpMaq1;

#endif