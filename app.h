#ifndef __APP_H__
#define __APP_H__

void appInit(void);
void appCall(void);
void appInitHostIp(void);
unsigned char appIsConnected(void);
void acionadorDec(void);
void appEnableTCP(void);
void appSendConnect(void);
unsigned char appIsDhcp(void);

typedef struct appState
{
  char inputBuffer[10];
}uip_appstate_st;

#ifndef UIP_APPCALL
#define UIP_APPCALL appCall
#endif

#define UIP_APPSTATE_SIZE sizeof(uip_appstate_st)

#include "uipopt.h"
void appSendSyn(u16_t * ipaddr);
void appSetDnsAddr(u16_t *);

#endif