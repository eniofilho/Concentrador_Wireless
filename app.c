#include <intrinsics.h>
#include <string.h>
#include "io430.h"
#include "app.h"
#include "uip.h"
#include "appConf.h"
#include "serial.h"
#include "io.h"
#include "flash.h"
#include "uip_arp.h"
#include "enc28j60.h"
#include "utils.h"
#include "lcd.h"
#include "wirelessOpMaq.h"

#define RST_PSEL P4SEL
#define RST_PDIR P4DIR
#define RST_PIN  P4IN
#define RST_BT BIT2

#define WEB_ADDR_SIZE   32

#pragma pack(1)
typedef struct
{
  unsigned char ipTipo;
  unsigned char ipEstatico[4];
  unsigned char ipServidor[4];
  unsigned char netMask[4];
  unsigned int porta;
  unsigned char tempoPacote;
  unsigned char tempoAcionador[4];
  unsigned char dns;
  unsigned short dnsAddr[2];
  unsigned short drAddr[2];
  unsigned char webAddr[WEB_ADDR_SIZE];
}MEMORY_CPY;

#pragma pack(1)
typedef struct
{
  unsigned char concId[2*SIZE_CONC_MAC];
  unsigned char tipo;
  unsigned char status;
}CONC_ST;

#pragma pack(1)
typedef struct
{
  unsigned char num;
  unsigned char identificador[SIZE_SENS_ID];
  unsigned char tipo[SIZE_SENS_TIPO];
  unsigned char valor[SIZE_SENS_VAL];
}SENSOR_ST;

#pragma pack(1)
typedef struct
{
    unsigned char init;
    unsigned char estado[NUM_RELES];
}RELE_ST;

#pragma pack(1)
typedef struct
{
  unsigned char initString;
  CONC_ST concentrador;
  SENSOR_ST sensor[NUM_SENS];
  RELE_ST rele;
  unsigned char endString;
}DATA_ST;

//Protótipo das funções de escopo local
static int checkResetBt(void);
static void appWriteFlashData(void);
static void appReadFlashData(void);
static void appEraseFlashData(void);
static void appCheckFlashData(void);
static void sendError(void);
static void sendSensorData(void);
//static void sendOK(void);
static void rcvSensorData(u8_t volatile *, unsigned char);

/*!  \brief Endereço MAC */
#pragma segment="MAC_ADDR"
const volatile unsigned char * MAC_CONC = (unsigned char const volatile *)0x8000; //{0x00,0x00,0x00,0x07};

/*!  \brief Guarda as informações não voláteis armazenadas na flash */
MEMORY_CPY memoryData;

/*!  \brief Indica se a conexão está ativa */
unsigned char connected;

/*!  \brief Armazena o tempo para desligar um determinado acionador */
unsigned char acionadorCount[4];

/*!  \brief Informa que uma mensagem acaba de ser transmitida */
unsigned char dataSent = 0;

/*!  \brief */
unsigned char repetAck = 0;

/*!  \brief Número de ciclos até que uma nova mensagem seja envia para o servidor*/
static unsigned char ciclos = 0;

/*!  \brief Guarda número de ciclos transcorridos */
unsigned char countBaud = 0;

/*! \brief Endereço MAC com a informação do identificador do controlador */
static struct uip_eth_addr ethaddr;

/*! \brief Buffer onde será armazenada a mensagem a ser enviada para o servidor */
unsigned char pbuffer[sizeof(DATA_ST)];

/*!  \brief Ponteiro para os dados a serem enviados */
DATA_ST * protoBuffer = ((DATA_ST *)&pbuffer[0]);

/*!  \brief Endereço do servidor de DNS */
u16_t dnsAddr[2];

extern unsigned char enableTCP;
extern unsigned char reboot;

/*!  \brief Inicia as estruturas com os dados armazenados na memória interna não-volátil */
void appInitHostIp(void)
{
  volatile u16_t addr[2];

  //Configura endereço ip local
  addr[0] = HTONS((((u16_t)memoryData.ipEstatico[0])<< 8) + memoryData.ipEstatico[1]);
  addr[1] = HTONS((((u16_t)memoryData.ipEstatico[2])<< 8) + memoryData.ipEstatico[3]);
  uip_sethostaddr(addr);

  addr[0] = HTONS((((u16_t)memoryData.netMask[0])<< 8) + memoryData.netMask[1]);
  addr[1] = HTONS((((u16_t)memoryData.netMask[2])<< 8) + memoryData.netMask[3]);
  uip_setnetmask(addr);

  addr[0] = memoryData.drAddr[0];
  addr[1] = memoryData.drAddr[1];
  uip_setdraddr(addr);
}

/*!  \brief Inicia aplicação */
void appInit(void)
{
    u16_t dns[2];

   //Verifica se o botão de reset está pressionado
   if(checkResetBt())
   {
      appEraseFlashData();
   }

   //Adquire os dados da memória não volátil
   appReadFlashData();

   //Verifica a consistência dos dados armazenados na memória flash
   appCheckFlashData();

   if(memoryData.ipTipo == '0')
   {
      //Inicia o DHCP
      ethaddr.addr[0] = ENC28J60_MAC0;
      ethaddr.addr[1] = ENC28J60_MAC1;
      ethaddr.addr[2] = MAC_CONC[0];
      ethaddr.addr[3] = MAC_CONC[1];
      ethaddr.addr[4] = MAC_CONC[2];
      ethaddr.addr[5] = MAC_CONC[3];
      uip_setethaddr(ethaddr);
      dhcpc_init(&ethaddr, sizeof(ethaddr));
   }
   else
   {
      //Inicia com ip fixo
      appInitHostIp();
      dns[0] = memoryData.dnsAddr[0];
      dns[1] = memoryData.dnsAddr[1];
      appSetDnsAddr(dns);
      appSendConnect();
   }

   ciclos = memoryData.tempoPacote/5;
}

/*!  \brief Executa a aplicação que controla a conexão de rede */
void appCall(void)
{
  //Se conexão foi abortada envia reset e reboot do pc
  if(uip_aborted())
  {
      reboot = 1;
      connected = 0;
      dataSent = 0;
  }

  //Verifica se houve algum problema na conexão
  if(uip_timedout())
  {
    //Flag que indica erro na conexão
    connected = 0;
    //lcdPutString("D",1,15);
    dataSent = 0;
    enc28j60_init();

    //appSendConnect();
    if(memoryData.ipTipo == '0')
    {
       //Inicia o DHCP
       ethaddr.addr[0] = ENC28J60_MAC0;
       ethaddr.addr[1] = ENC28J60_MAC1;
       ethaddr.addr[2] = MAC_CONC[0];
       ethaddr.addr[3] = MAC_CONC[1];
       ethaddr.addr[4] = MAC_CONC[2];
       ethaddr.addr[5] = MAC_CONC[3];
       uip_setethaddr(ethaddr);
       dhcpc_init(&ethaddr, sizeof(ethaddr));
    }
    else
    {
       u16_t dns[2];

       //Inicia com ip fixo
       appInitHostIp();
       dns[0] = memoryData.dnsAddr[0];
       dns[1] = memoryData.dnsAddr[1];
       appSetDnsAddr(dns);
       appSendConnect();
    }
  }

  if(uip_closed())
  {
     WDTCTL = 0;
  }

  //Caso a conexão com o servidor acabou de ocorrer
  if(uip_connected())
  {
    connected = 1;
    //lcdPutString("C",1,15);
    dataSent = 0;
  }

  //Recebe os dados enviados pelo servidor
  if(uip_newdata() && connected)
  {
    rcvSensorData(uip_appdata,uip_len);
  }

  if(connected && (uip_acked()) && dataSent && (!repetAck))
  {
    dataSent = 0;

    //Diz que valor dos sensores já foi enviado
    sensorClear();
  }

  if(uip_rexmit() && connected && (uip_len == 0))
  {
    //Evita travamento da interface de rede
    enc28j60_init();

    //Envia os dados do sensor no caso de retransmissão
    sendSensorData();

    return;
  }

  //Transmite dados para o servidor
  if(uip_poll() && connected && (uip_len==0))
  {
    //Verifica se existem dados do sensor wireless pendentes
    if(wOpMaq1.wirelessMsgReceived(&wOpMaq1))
    {
      countBaud = 0;
      sendSensorData();
    }
    else if(memoryData.tempoPacote != 0)
    {
      //Envia os dados dos sensores caso seja modo síncrono
      if(ciclos >= 1)
      {
        if(countBaud >= (ciclos -1))
        {
           countBaud = 0;
           sendSensorData();
           dataSent = 1;
        }
        else
        {
           countBaud++;
        }
      }
    }
  }
}

/*!  \brief Verifica se o botão de reset foi pressionado */
static int checkResetBt(void)
{
  int i;

  RST_PSEL &= ~RST_BT;
  RST_PDIR &= ~RST_BT;

  for(i=0;i<100;i++)
  {
    if(RST_PIN & RST_BT)
    {
      return 0;
    }
  }

  return 1;
}

/*!  \brief Verifica se é necessário desligar algum relé */
void acionadorDec(void)
{
    unsigned char i;

    for(i = 0;i<4;i++)
    {
        if(acionadorCount[i] != 0)
        {
            acionadorCount[i]--;
            if(acionadorCount[i] == 0)
            {
                ioReleClear(i+1);
            }
        }
    }
}

/*!  \brief Atualiza a flash com os parâmetros modificados */
static void appWriteFlashData(void)
{
   __disable_interrupt();

  //Apaga a memória
  flashClr((unsigned int *)FLASH_INIT_ADDR);

  //Escreve os dados na memória
  flashWriteBuff((unsigned char *)FLASH_INIT_ADDR, (unsigned char *)&memoryData, sizeof(MEMORY_CPY));

   __enable_interrupt();
}

/*!  \brief Le os dados da flash e salva em uma cópia na RAM */
static void appReadFlashData(void)
{
  unsigned char count = sizeof(MEMORY_CPY);
  unsigned char * ptr;
  unsigned char * ram;
  unsigned char i;

  ptr = (unsigned char *) FLASH_INIT_ADDR;
  ram = (unsigned char *)&memoryData;
  for(i=0;i<count;i++)
  {
    * ram = * ptr;
    ram++;
    ptr++;
  }
}

/*!  \brief Apaga todo o setor da flash */
static void appEraseFlashData(void)
{
  __disable_interrupt();

  //Apaga a memória
  flashClr((unsigned int *)FLASH_INIT_ADDR);

   __enable_interrupt();
}

/*!  \brief Verifica a coerência dos dados salvos na flash */
static void appCheckFlashData(void)
{
  if((memoryData.ipTipo != '0') && (memoryData.ipTipo != '1'))
  {
     //Ip fixo
     memoryData.ipTipo = '1';
    
     //DHCP
     //memoryData.ipTipo = '0';
  }

  if((memoryData.ipEstatico[0] == 255) &&
     (memoryData.ipEstatico[1] == 255) &&
     (memoryData.ipEstatico[2] == 255) &&
     (memoryData.ipEstatico[3] == 255))
  {
    memoryData.ipEstatico[0] = UIP_IPADDR0;
    memoryData.ipEstatico[1] = UIP_IPADDR1;
    memoryData.ipEstatico[2] = UIP_IPADDR2;
    memoryData.ipEstatico[3] = UIP_IPADDR3;
  }

  if((memoryData.netMask[0] == 255) &&
     (memoryData.netMask[1] == 255) &&
     (memoryData.netMask[2] == 255) &&
     (memoryData.netMask[3] == 255))
  {
    memoryData.netMask[0] = UIP_NETMASK0;
    memoryData.netMask[1] = UIP_NETMASK1;
    memoryData.netMask[2] = UIP_NETMASK2;
    memoryData.netMask[3] = UIP_NETMASK3;
  }

  if((memoryData.ipServidor[0] == 255) &&
     (memoryData.ipServidor[1] == 255) &&
     (memoryData.ipServidor[2] == 255) &&
     (memoryData.ipServidor[3] == 255))
  {
    memoryData.ipServidor[0] = IP_SERVIDOR_0;
    memoryData.ipServidor[1] = IP_SERVIDOR_1;
    memoryData.ipServidor[2] = IP_SERVIDOR_2;
    memoryData.ipServidor[3] = IP_SERVIDOR_3;
  }

  if(memoryData.porta == 0xFFFF)
  {
    memoryData.porta = PORTA;
  }

  if(memoryData.tempoPacote == 255)
  {
    memoryData.tempoPacote = 100;
  }

  if((memoryData.dns > 1))
  {
      memoryData.dns = 0;
  }

  if((memoryData.dnsAddr[0] == 0xFFFF) && (memoryData.dnsAddr[1] == 0xFFFF))
  {
      //Endereço do servidor DNS padrão 8.8.8.8
      memoryData.dnsAddr[0] = 0x0808;
      memoryData.dnsAddr[1] = 0x0808;
  }

  if((memoryData.drAddr[0] == 0xFFFF) && (memoryData.drAddr[1] == 0xFFFF))
  {
      memoryData.drAddr[0] = HTONS((UIP_DRIPADDR0 << 8) | UIP_DRIPADDR1);
      memoryData.drAddr[1] = HTONS((UIP_DRIPADDR2 << 8) | UIP_DRIPADDR3);
  }
}

/*!  \brief Verifica se a aplicação está conectada */
unsigned char appIsConnected(void)
{
    return connected;
}

/*!  \brief Coloca na pilha TCP os dados que serão enviados do sensor */
static void sendSensorData(void)
{
  unsigned char i;

  if(wOpMaq1.wirelessMsgReceived(&wOpMaq1))
  {
    //////////////////////
    //Envia mensagem do leitor wireless
    /////////////////////
    
    //Coloca os dados na pilha TCP/IP
    uip_send((&(wOpMaq1.serialBuffer[0])),wOpMaq1.ptSerial);
    
    //Reinicia a máquina de recepção do wireless
    wOpMaq1.cleanRx(&wOpMaq1);
  }
  else
  {
    ///////////////////////////
    //Envia mensagem do leitor cabeado
    ///////////////////////////
    
    protoBuffer->initString = '<';

    //Preenche dados fixos da estrutura
    protoBuffer->concentrador.concId[0] = hexToChar((MAC_CONC[0] >> 4) & 0x0F);
    protoBuffer->concentrador.concId[1] = hexToChar(MAC_CONC[0] & 0x0F);
    protoBuffer->concentrador.concId[2] = hexToChar((MAC_CONC[1] >> 4) & 0x0F);
    protoBuffer->concentrador.concId[3] = hexToChar(MAC_CONC[1] & 0x0F);
    protoBuffer->concentrador.concId[4] = hexToChar((MAC_CONC[2] >> 4) & 0x0F);
    protoBuffer->concentrador.concId[5] = hexToChar(MAC_CONC[2] & 0x0F);
    protoBuffer->concentrador.concId[6] = hexToChar((MAC_CONC[3] >> 4) & 0x0F);
    protoBuffer->concentrador.concId[7] = hexToChar(MAC_CONC[3] & 0x0F);
    protoBuffer->concentrador.tipo = hexToChar(CONC_TIPO);

    if(sensorGetConcStatus())
    {
      protoBuffer->concentrador.status = '1';
    }
    else
    {
      protoBuffer->concentrador.status = '0';
    }

    for(i=0;i<NUM_SENS;i++)
    {
      protoBuffer->sensor[i].num = hexToChar(i+1);
      sensorGetId(i,&(protoBuffer->sensor[i].identificador));
      sensorGetTipo(i,&(protoBuffer->sensor[i].tipo));
      sensorGetValor(i,&(protoBuffer->sensor[i].valor));
    }

    //Estado dos reles
    protoBuffer->rele.init = 'R';

    for(i=0;i<4;i++)
    {
      unsigned char releTmp;

      releTmp = ioReleStatus(i);
      if(releTmp != '0')
      {
        protoBuffer->rele.estado[i] = releTmp - i;
      }
      else
      {
        protoBuffer->rele.estado[i] = '0';
      }
    }
    protoBuffer->endString = '>';

    //Envia os dados
    uip_send(pbuffer,sizeof(DATA_ST));

    //Diz que dados já foram enviados
    sensorSetDataSent();
  }
}

static void rcvSensorData(u8_t volatile * data, unsigned char len)
{
  if(data[0] == '<')
  {
    switch(data[1])
    {
      case 'S':
        //<S:N,Sn>
        if((data[2] == ':') && ((data[3] - '0') < (NUM_SENS + 1)) && (data[4] == ','))
        {
        }
        else
        {
          sendError();
        }
      break;

      case 'A':
        //<A:N,An,T>
        if((data[2] == ':') && ((data[3] - '0') <= (NUM_ACIONADORES)) && (data[4] == ',') && (data[6] == ','))
        {
           //Seta o valor do acionador
           unsigned char num = data[3] - '0';
           unsigned char val = data[5] - '0';
           unsigned char tempo = 0;
           unsigned char i;

           i = 7;
           do
           {
              if((data[i] < '0') || (data[i] > '9'))
              {
              }
              else
              {
                 tempo *= 10;
                 tempo += (data[i] - '0');
                 i++;
              }
           }
           while(((data[i] >= '0') && (data[i] <= '9')) && (i < 11));

           if(i < 11)
           {
               if(!val)
               {
                 ioReleClear(num);
               }
               else if(val == 1)
               {
                 ioReleSet(num);

                 //Guardar valor do tempo de acionamento
                 //memoryData.tempoAcionador[num-1] = tempo << 1;
                 acionadorCount[num-1] = tempo << 1;
               }
           }
           else
           {
               //Erro no pacote de dados
               sendError();
           }
        }
        else
        {
            sendError();
        }
      break;

      case 'I':
        //<I:T:PPP.PPP.P.PPP>
        if((data[2] == ':') && ((data[3] == '0') || (data[3] == '1')) && (data[4] == ','))
        {
          unsigned char check;

          //Salva indicador do tipo de configuração DHCP ou estático
          memoryData.ipTipo = data[3];

          //Salva ip no caso de ip estático
          if(data[3] == '1')
          {
            unsigned char i;
            unsigned char ip;
            unsigned char count;

            ip = 0;
            count = 5;
            check = 0;
            for(i=0;i<4;i++)
            {
              do
              {
                if((data[count] < '0') || (data[count] > '9'))
                {
                  check=1;
                }
                ip *= 10;
                ip += (data[count] - '0');
                count++;
              }
              while((data[count] >= '0') && (data[count] <= '9'));

              count++;
              memoryData.ipEstatico[i] = ip;
              ip = 0;
            }
          }
          else
          {
            appWriteFlashData();
          }

          if(!check)
          {
             appWriteFlashData();
          }
        }
      break;

      case 'V':
        //<V>
        if(data[2] == '>')
        {
            //Monta o pacote de versão
            pbuffer[0] = '<';
            pbuffer[1] = 'V';
            pbuffer[2] = VERSION_MAJOR;
            pbuffer[3] = '.';
            pbuffer[4] = VERSION_MINOR;
            pbuffer[5] = '>';
            uip_send(pbuffer,6);
        }
        else
        {
            sendError();
        }
      break;

      case 'C':
        //<C:XXX.XXX.X.XXX,PPPP>
        //<C:www.servidor.com.br,PPPP>
         if(data[2] == ':')
         {
            unsigned char ip;
            unsigned char count;
            unsigned char i;

            if((data[3] >= '0') && (data[3] <= '9'))
            {
                memoryData.dns = 0;
                ip = 0;
                count = 3;
                for(i=0;i<4;i++)
                {
                    do
                    {
                        ip *= 10;
                        ip += (data[count] - '0');
                        count++;
                    }
                    while((data[count] >= '0') && (data[count] <= '9'));

                    memoryData.ipServidor[i] = ip;
                    ip = 0;
                    count++;
                }
                count--;
            }
            else
            {
                unsigned char countAdd;

                //Endereço será resolvido por dns
                memoryData.dns = 1;

                //Limpa memoria
                for(countAdd=0;countAdd<WEB_ADDR_SIZE;countAdd++)
                {
                    memoryData.webAddr[countAdd] = 0;
                }

                countAdd = 0;
                count = 3;
                //21 e 7F são valores imprimíveis na tablea ASCII incluindo letras maiúsculas, minúsculas e pontos
                while((data[count] != ',') && (data[count] > 0x21) && (data[count] < 0x7F) && (countAdd < WEB_ADDR_SIZE))
                {
                    memoryData.webAddr[countAdd] = data[count];
                    countAdd++;
                    count++;
                }
            }

            if(data[count] == ',')
            {
              unsigned int porta;
              count++;

              porta = 0;
              do
              {
                porta *=10;
                porta+=(data[count] - '0');
                count++;
              }
              while((data[count] >= '0') && (data[count] <= '9'));

              memoryData.porta = porta;
            }

            appWriteFlashData();
        }
        else
        {
            sendError();
        }
      break;

      case 'T':
        //<T:XXX>
        //XXX - Tempo entre pacotes de 0 a 256 décimos de segundo
        if(data[2] == ':')
        {
            unsigned char time;
            unsigned char count;

            time = 0;
            count = 3;
            do
            {
               time *= 10;
               time += (data[count] - '0');
               count++;
            }
            while((data[count] >= '0') && (data[count] <= '9'));

            memoryData.tempoPacote = time;
            ciclos = time/5;

            appWriteFlashData();
         }
         else
         {
            sendError();
         }
      break;

      case 'R':
        //<R>
        //Reinicia a aplicação
        uip_close();
        connected = 0;
        reboot = 1;
      break;

      case 'D':
        //<D>
        sendSensorData();
      break;

      case 'P':
        //<P>
        //Pacote de ping
        if(data[2] == '>')
        {
            //Monta o pacote de versão
            pbuffer[0] = '<';
            pbuffer[1] = 'A';
            pbuffer[2] = 'C';
            pbuffer[3] = 'K';
            pbuffer[4] = '>';

            uip_send(pbuffer,5);
        }
        else
        {
            sendError();
        }
      break;

      case 'N':
        //<N:TTT.TTT.TTT.TTT>
        if((data[2] == ':'))
        {
           unsigned char check;
           unsigned char i;
           unsigned char ip[4];
           unsigned char count;

           count = 3;
           check = 0;
           memoryData.dnsAddr[0] = 0;
           memoryData.dnsAddr[1] = 0;

           for(i=0;i<4;i++)
           {
              ip[i] = 0;

              do
              {
                if((data[count] < '0') || (data[count] > '9'))
                {
                  check=1;
                }
                ip[i] *= 10;
                ip[i] += (data[count] - '0');
                count++;
              }
              while((data[count] >= '0') && (data[count] <= '9'));

              count++;
            }

            memoryData.dnsAddr[0] = (unsigned int)ip[1];
            memoryData.dnsAddr[0] = memoryData.dnsAddr[0] << 8;
            memoryData.dnsAddr[0] += ip[0];
            memoryData.dnsAddr[1] = (unsigned int)ip[3];
            memoryData.dnsAddr[1] = memoryData.dnsAddr[1] << 8;
            memoryData.dnsAddr[1] += ip[2];

            if(!check)
            {
               appWriteFlashData();
            }
        }
      break;

      case 'B':
        //<B:TTT.TTT.TTT.TTT>
        if((data[2] == ':'))
        {
           unsigned char check;
           unsigned char i;
           unsigned char ip[4];
           unsigned char count;

           count = 3;
           check = 0;
           memoryData.drAddr[0] = 0;
           memoryData.drAddr[1] = 0;

           for(i=0;i<4;i++)
           {
              ip[i] = 0;

              do
              {
                if((data[count] < '0') || (data[count] > '9'))
                {
                  check=1;
                }
                ip[i] *= 10;
                ip[i] += (data[count] - '0');
                count++;
              }
              while((data[count] >= '0') && (data[count] <= '9'));

              count++;
            }

            //Grava valor do default router na memória
            memoryData.drAddr[0] = (unsigned int)ip[1];
            memoryData.drAddr[0] = memoryData.drAddr[0] << 8;
            memoryData.drAddr[0] += ip[0];
            memoryData.drAddr[1] = (unsigned int)ip[3];
            memoryData.drAddr[1] = memoryData.drAddr[1] << 8;
            memoryData.drAddr[1] += ip[2];

            if(!check)
            {
               appWriteFlashData();
            }
        }
      break;

      case 'M':
        //<M:XXX.XXX.X.XXX>
         if(data[2] == ':')
         {
            unsigned char ip;
            unsigned char count;
            unsigned char i;

            ip = 0;
            count = 3;
            for(i=0;i<4;i++)
            {
              do
              {
                ip *= 10;
                ip += (data[count] - '0');
                count++;
              }
              while((data[count] >= '0') && (data[count] <= '9'));

              memoryData.netMask[i] = ip;
              ip = 0;
              count++;
            }
            appWriteFlashData();
        }
        else
        {
            sendError();
        }
      break;
      
      //Comandos para o concentrador do tipo wireless
      case 'W':
        if(data[2] == ':')
        {
           switch(data[3])
           {
              //Comandos para configuração dos sensores
              case 'S':
                if(data[4] == 'W')
                {
                  //Comando para gravar o identificador de um sensor na lista
                  //sendOK();
                  wOpMaq1.writeSens(&wOpMaq1,(unsigned char *)&data[5]);
                }
                else if(data[4] == 'E')
                {
                  //Comando para apagar o identificador de um sensor na lista
                  //sendOK();
                  wOpMaq1.eraseSens(&wOpMaq1,(unsigned char *)&data[5]);
                }
                else if(data[4] == 'L')
                {
                  //Comando para listar os sensores presentes na lista
                  //sendOK();
                  wOpMaq1.listSens(&wOpMaq1);
                }
                else
                {
                  //Comando inválido
                  sendError();
                }
              break;
              
              //Comandos para configuração dos canais de RF
              case 'C':
                if(data[4] == 'S')
                {
                  //Comando para setar o canal de RF
                  //sendOK();
                  wOpMaq1.changeCh(&wOpMaq1,data[5]);
                }
                else if(data[4] == 'R')
                {
                  //Comando para ler o canal de RF
                  //sendOK();
                  wOpMaq1.readCh(&wOpMaq1);
                }
                else
                {
                  //Comando inválido
                  sendError();
                }
              break;
             
              //Comandos para configuração do modo de operação do concentrador wireless
              case 'M':
                if(data[4] == 'S')
                {
                  //Comando para configurar o concentrador para o modo busca
                  //sendOK();
                  wOpMaq1.setMode(&wOpMaq1,MODE_SEARCH);
                }
                else if(data[4] == 'R')
                {
                  //Comando para configurar o concentrador para o modo de recepção
                  //sendOK();
                  wOpMaq1.setMode(&wOpMaq1,MODE_RX);
                }
                else if(data[4] == 'I')
                {
                  //Comando para configurar o concentrador para o modo de inventário
                  //sendOK();
                  wOpMaq1.setMode(&wOpMaq1,MODE_INV);
                }
                else
                {
                  //Comando inválido
                  sendError();
                }
              break;
              
              default:
                //Comando inválido
                sendError();
              break;
           }
        }
      break;
    }
  }
  else
  {
    sendError();
  }
}

static void sendError(void)
{
    //Envia a string indicando erro no comando recebido
    pbuffer[0] = 'E';
    pbuffer[1] = 'R';
    pbuffer[2] = 'R';
    pbuffer[3] = 'O';
    pbuffer[4] = 'R';

    uip_send(pbuffer,5);
}

/*
static void sendOK(void)
{
    //Envia a string indicando que comando foi recebido com sucesso
    pbuffer[0] = 'O';
    pbuffer[1] = 'K';
    
    uip_send(pbuffer,2);
}
*/

void appEnableTCP(void)
{
    enableTCP = 1;
}

void appSendConnect(void)
{
    u16_t ipaddr[2];
    u16_t * ipPt;

    if(memoryData.dns == 0)
    {
       //Envia comando de conexão para endereço do servidor guardado
       uip_ipaddr(ipaddr, memoryData.ipServidor[0],memoryData.ipServidor[1],memoryData.ipServidor[2],memoryData.ipServidor[3]);
       uip_connect(ipaddr, HTONS(memoryData.porta));
       appEnableTCP();
    }
    else
    {
        //Requisita o endereço do servidor pelo dns
        ipPt = resolv_lookup((char *)memoryData.webAddr);
        if(ipPt == NULL)
        {
            resolv_conf(dnsAddr);
            resolv_query((char *)memoryData.webAddr);
        }
        else
        {
            uip_connect(ipPt, HTONS(memoryData.porta));
            appEnableTCP();
        }
    }
}

void appSendSyn(u16_t * ipaddr)
{
     uip_connect(ipaddr, HTONS(memoryData.porta));
}

void appSetDnsAddr(u16_t * ipaddr)
{
    dnsAddr[0] = ipaddr[0];
    dnsAddr[1] = ipaddr[1];
}

unsigned char appIsDhcp(void)
{
    if(memoryData.ipTipo != '0')
    {
       return 0;
    }

    return 1;
}