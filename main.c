#include <intrinsics.h>
#include "io430.h"
#include "wtd.h"
#include "clock.h"
#include "lcd.h"
#include "serial.h"
#include "enc28j60.h"
#include "io.h"
#include "uip_arp.h"
#include "uip.h"
#include "utils.h"
#include "resolv.h"
#include "wirelessOpMaq.h"

/*! \brief Ponteiro onde os dados recebidos são armazenados */
#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

static int start;
static int current;
static unsigned int contReboot = 0;
unsigned char enableTCP = 0;
unsigned char lcdState = 2;
unsigned char reboot;
unsigned int countLcdInit = 0;
unsigned char lcdCon = 0;

static int rtTicks(void)
{
  extern int ticks;

  return ticks;
}

void main(void)
{
  //Para o watchdog
  wtdStop();

  /*
  __disable_interrupt();
  flashWbSegA((unsigned char *)0x10F8, 0x88);
  flashWbSegA((unsigned char *)0x10F9, 0x8F);
   __enable_interrupt();
  */

  //Configura clock
  clockInit();

  //Watchdog pode agir a partir da iniciação dos periféricos
  wtdClear();

  //Configura timer responsável pela comunicação serial com os sensores
  serialInit();

  //Inicia a interface com o controlador ethernet
  enc28j60_init();

  //Configura o estado dos acionadores
  ioInit();
  
  //Inicia a máquina de recepção com o rádio
  wOpMaq1.init(&wOpMaq1);
  
  //Inicia o TCP/IP Stack
  uip_init();
  uip_arp_init();

  //Inicia o DNS
  resolv_init();

  //Inicia a aplicacao
  appInit();

  //Configura o LCD
  //lcdWait();
  //lcdWait();
  lcdInit();
#ifndef DEBUG_SENSOR
  lcdPutString("SMARTFIX        ",0,0);
#endif
  lcdPutString("POWER-ON: OK   D",1,0);

  //Coloca o endereço MAC do concentrador no lcd
  lcdSetChar(hexToChar((MAC_CONC[0] & 0xF0) >> 4),8,0);
  lcdSetChar(hexToChar(MAC_CONC[0] & 0x0F),9,0);
  lcdSetChar(hexToChar((MAC_CONC[1] & 0xF0) >> 4),10,0);
  lcdSetChar(hexToChar(MAC_CONC[1] & 0x0F),11,0);
  lcdSetChar(hexToChar((MAC_CONC[2] & 0xF0) >> 4),12,0);
  lcdSetChar(hexToChar(MAC_CONC[2] & 0x0F),13,0);
  lcdSetChar(hexToChar((MAC_CONC[3] & 0xF0) >> 4),14,0);
  lcdSetChar(hexToChar(MAC_CONC[3] & 0x0F),15,0);

  //Habilita o watchdog
  wtdClear();

  //Habilita a flag global de interrupções
  __enable_interrupt();

  start = rtTicks();

  while(1)
  {
    unsigned char i;

#ifdef DEBUG_SENSOR
	unsigned char lcdDebug[7];
#endif
        
    //Verifica se existem dados pendentes do wireless
    wOpMaq1.rxData(&wOpMaq1);
    
    //Verifica o watchdog
    wtdTest();

    current = rtTicks();
    if((current - start) >= 6)
    {
#ifdef DEBUG_SENSOR
        lcdDebug[0] = 0;
        lcdDebug[1] = 0;
        lcdDebug[2] = 0;
        lcdDebug[3] = 0;
        lcdDebug[4] = 0;
        lcdDebug[5] = 0;
        lcdDebug[6] = 0;

        sensorDebugValor(0,lcdDebug);
        lcdPutString(lcdDebug,0,0);
#endif
       start = current;

       if(enableTCP)
       {
          for(i = 0;i< UIP_CONNS; i++)
          {
             uip_periodic(0);
             if(uip_len > 0)
             {
               uip_arp_out();
               enc28j60_send();
             }
          }
       }

       for(i = 0; i < UIP_UDP_CONNS; i++)
       {
         uip_udp_periodic(i);
         if(uip_len > 0)
         {
           uip_arp_out();
           enc28j60_send();
         }
       }

       if(lcdCon)
       {
           if((!(enc28j60PhyRead(PHSTAT2) && 0x0400)) || (!appIsConnected()))
           {
               lcdPutString("D",1,15);
               lcdCon = 0;
           }
       }
       else
       {
           if((enc28j60PhyRead(PHSTAT2) && 0x0400) && appIsConnected())
           {
               lcdPutString("C",1,15);
               lcdCon= 1;
           }
       }

       //Verifica status dos reles
       acionadorDec();
    }
    else if((uip_len = enc28j60_poll()) > 0)
    {
      if(BUF->type == htons(UIP_ETHTYPE_IP))
      {
        uip_arp_ipin();
	uip_input();
	if(uip_len > 0)
        {
	  uip_arp_out();
	  enc28j60_send();
	}
      }
      else if(BUF->type == htons(UIP_ETHTYPE_ARP))
      {
	uip_arp_arpin();
	if(uip_len > 0)
        {
	  enc28j60_send();
	}
      }
    }

    if(reboot)
    {
      contReboot++;
      if(contReboot > 1000)
      {
        WDTCTL = 0;
      }
    }

    //Atualiza o LCD se necesário
    switch(lcdState)
    {
      case 0:
        for(i=0;i<10;i++)
        {
           unsigned char atual;
           unsigned char anterior;

           atual = sensorIsSet(i);
           anterior = lcdGetChar(i);

           if(atual != anterior)
           {
             lcdSetChar(atual,i,1);
             lcdState = 1;
           }
         }

         if(ioReleChanged())
         {
             //Atualiza o valor dos relés
             lcdSetChar(' ',10,1);
             lcdSetChar(ioReleStatus(0),11,1);
             lcdSetChar(ioReleStatus(1),12,1);
             lcdSetChar(ioReleStatus(2),13,1);
             lcdSetChar(ioReleStatus(3),14,1);

             lcdState = 1;
         }
      break;

      case 1:
        if(lcdLowUpdate())
        {
          lcdState = 0;
        }
      break;

      case 2:
        //Estado de espera da mensagem inicial
        if(countLcdInit > 30000)
        {
           lcdState = 0;
        }
      break;
    }
  }
}