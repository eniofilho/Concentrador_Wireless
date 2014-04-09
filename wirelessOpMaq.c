/* \file wirelessOpMaq.c
 * \brief Máquina que controla as mensagem do dispositivo wireless
 */

#include "wirelessOpMaq.h"
#include "utils.h"
#include "appConf.h"
#include "serial.h"
#include "lcd.h"
#include "app.h"
   
// prototipos dos metodos
void wOpMaqInit(void *);
void wOpMaqSetMode(void *, W_MODE);
void wOpMaqWriteSens(void *, unsigned char *);
void wOpMaqEraseSens(void *, unsigned char *);
void wOpMaqListSens(void *);
void wOpMaqChangeCh(void *, unsigned char);
void wOpMaqReadCh(void *);
void wOpMaqCleanRx(void *);
void wOpMaqRxData(void *);
unsigned char wOpMaqWirelessMsgReceived(void *);

// Instancias de porta serial
W_OP_MAQ wOpMaq1 = {wOpMaqInit};

//Timeout para a máquina de recepçao
unsigned int countWireless = 0;

/* \brief Inicializacao dos metodos  e variaveis do objeto. */
void wOpMaqInit(void * pwOp)
{
   W_OP_MAQ * wOp = (W_OP_MAQ *)pwOp;
   
   //Métodos
   wOp->setMode                 = wOpMaqSetMode;
   wOp->writeSens               = wOpMaqWriteSens;
   wOp->eraseSens               = wOpMaqEraseSens;
   wOp->listSens                = wOpMaqListSens;
   wOp->changeCh                = wOpMaqChangeCh;
   wOp->readCh                  = wOpMaqReadCh;
   wOp->cleanRx                 = wOpMaqCleanRx;
   wOp->wirelessMsgReceived     = wOpMaqWirelessMsgReceived;
   wOp->rxData                  = wOpMaqRxData;
      
   //Configura as variáveis da classe
   wOp->uart = &uart1;
   wOp->state = RX_IDLE;
   wOp->msgType = W_MSG_NONE;
   wOp->ptSerial = 0;
   
   //Configura a porta serial
   wOp->uart->init(wOp->uart);
   wOp->uart->reset(wOp->uart);
   wOp->uart->config(wOp->uart,UART_SPEED_115200,UART_BITS_8,UART_PARITY_NONE,UART_STOP_BITS_1);
   wOp->uart->start(wOp->uart);
}

/* \brief Configura o modo de operação do mcu da interface wireless */
void wOpMaqSetMode(void * pwOp, W_MODE mode)
{
  W_OP_MAQ * wOp = (W_OP_MAQ *)pwOp;
  
  switch(mode)
  {
    case MODE_RX:
      wOp->uart->putBuffTx(wOp->uart,'M');
      wOp->uart->putBuffTx(wOp->uart,'R');      
    break;
    
    case MODE_SEARCH:
      wOp->uart->putBuffTx(wOp->uart,'M');
      wOp->uart->putBuffTx(wOp->uart,'S');
    break;
    
    case MODE_INV:
      wOp->uart->putBuffTx(wOp->uart,'M');
      wOp->uart->putBuffTx(wOp->uart,'I');
    break;
    
    default:
    break;
  }
}

/* \brief Grava um sensor na lista de sensores. */
void wOpMaqWriteSens(void * pwOp, unsigned char * id)
{
  W_OP_MAQ * wOp = (W_OP_MAQ *)pwOp;
  
  wOp->uart->putBuffTx(wOp->uart,'S');
  wOp->uart->putBuffTx(wOp->uart,'W');
  wOp->uart->putBuffTx(wOp->uart,charToHex(&id[0]));
  wOp->uart->putBuffTx(wOp->uart,charToHex(&id[2]));
  wOp->uart->putBuffTx(wOp->uart,charToHex(&id[4]));
  wOp->uart->putBuffTx(wOp->uart,charToHex(&id[6]));
}

/* \brief Apaga um sensor da lista de sensores. */
void wOpMaqEraseSens(void * pwOp, unsigned char * id)
{
  W_OP_MAQ * wOp = (W_OP_MAQ *)pwOp;
  
  wOp->uart->putBuffTx(wOp->uart,'S');
  wOp->uart->putBuffTx(wOp->uart,'E');
  wOp->uart->putBuffTx(wOp->uart,charToHex(&id[0]));
  wOp->uart->putBuffTx(wOp->uart,charToHex(&id[2]));
  wOp->uart->putBuffTx(wOp->uart,charToHex(&id[4]));
  wOp->uart->putBuffTx(wOp->uart,charToHex(&id[6]));
}

/* \brief Lista os identificadores de sensores presentes na lista. */
void wOpMaqListSens(void * pwOp)
{
  W_OP_MAQ * wOp = (W_OP_MAQ *)pwOp;
  
  wOp->uart->putBuffTx(wOp->uart,'S');
  wOp->uart->putBuffTx(wOp->uart,'L');
}

/* \brief Muda o canal de frequencia do concentrador. */
void wOpMaqChangeCh(void * pwOp, unsigned char ch)
{
  W_OP_MAQ * wOp = (W_OP_MAQ *)pwOp;
  
  wOp->uart->putBuffTx(wOp->uart,'C');
  wOp->uart->putBuffTx(wOp->uart,'S');
  wOp->uart->putBuffTx(wOp->uart,ch);
}

/* \brief Lê qual o canal de frequência do concentrador. */
void wOpMaqReadCh(void * pwOp)
{
  W_OP_MAQ * wOp = (W_OP_MAQ *)pwOp;
  
  wOp->uart->putBuffTx(wOp->uart,'C');
  wOp->uart->putBuffTx(wOp->uart,'R');
}

/* \brief Diz que a mensagem já foi enviada*/
void wOpMaqCleanRx(void * pwOp)
{
  W_OP_MAQ * wOp = (W_OP_MAQ *)pwOp;
  
  //Vai para o estado IDLE
  wOp->state = RX_IDLE;
  
  //Limpa a flag de envio de mensagens
  wOp->msgType = W_MSG_NONE;
}

/* \brief Verifica a existência de uma mensagem pendente pra ser processada*/
unsigned char wOpMaqWirelessMsgReceived(void * pwOp)
{
  W_OP_MAQ * wOp = (W_OP_MAQ *)pwOp;
  
  if(wOp->msgType != W_MSG_NONE)
  {
    return 1;
  }
  
  return 0;
}

/* \brief Máquina de estados para receber os dados. */
void wOpMaqRxData(void * pwOp)
{
  W_OP_MAQ * wOp = (W_OP_MAQ *)pwOp;
  unsigned char data;
  unsigned char ret;
  static unsigned char rxLine = 0;
  static unsigned char count = 0;
  
  if(wOp->state != RX_IDLE)
  {
    //Timeout de dois segundos
    if(countWireless > 24)
    {
        wOp->uart->reset(wOp->uart);
        wOp->state = RX_IDLE;
        wOp->ptSerial = 0;
        rxLine = 0;
        count = 0;
    }
  }
  else
  {
    countWireless = 0;
  }
  
  ret = wOp->uart->getBuffRx(wOp->uart,&data);
  if(ret)
  {
      if(data == '#')
      {
          //Reinicia a máquina
          wOp->uart->reset(wOp->uart);
          wOp->state = RX_IDLE;
          wOp->ptSerial = 0;
          rxLine = 0;
          count = 0;
      }
      else
      {
          switch(wOp->state)
          {
            case RX_IDLE:
                switch(data)
                {
                  case '(':
                    wOp->state = RX_MODE_INV;
                    
                    //Coloca cabeçalho da mensagem no buffer
                    wOp->serialBuffer[0] = '<';
                    wOp->serialBuffer[1] = 'W';
                    wOp->serialBuffer[2] = 'I';
                    wOp->serialBuffer[3] = hexToChar((MAC_CONC[0] >> 4) & 0x0F);
                    wOp->serialBuffer[4] = hexToChar(MAC_CONC[0] & 0x0F);
                    wOp->serialBuffer[5] = hexToChar((MAC_CONC[1] >> 4) & 0x0F);
                    wOp->serialBuffer[6] = hexToChar(MAC_CONC[1] & 0x0F);
                    wOp->serialBuffer[7] = hexToChar((MAC_CONC[2] >> 4) & 0x0F);
                    wOp->serialBuffer[8] = hexToChar(MAC_CONC[2] & 0x0F);
                    wOp->serialBuffer[9] = hexToChar((MAC_CONC[3] >> 4) & 0x0F);
                    wOp->serialBuffer[10] = hexToChar(MAC_CONC[3] & 0x0F);
                    wOp->serialBuffer[11] = hexToChar(CONC_TIPO);
                    if(sensorGetConcStatus())
                    {
                      wOp->serialBuffer[12] = '1';
                    }
                    else
                    {
                      wOp->serialBuffer[12] = '0';
                    }
                    
                    //Coloca a posição onde serão armazenados os demais dados
                    wOp->ptSerial = 13;
                  break;
                  
                  case '[':
                    wOp->state = RX_MODE_SEARCH;
                    wOp->ptSerial = 0;
                    
                    //Seta o contador de linhas do lcd recebidas
                    rxLine = 0;
                    count = 0;
                  break;
                  
                  case '<':
                    wOp->state = RX_MODE_RX;

                    //Coloca cabeçalho da mensagem no buffer
                    wOp->serialBuffer[0] = '<';
                    wOp->serialBuffer[1] = 'W';
                    wOp->serialBuffer[2] = 'R';
                    wOp->serialBuffer[3] = hexToChar((MAC_CONC[0] >> 4) & 0x0F);
                    wOp->serialBuffer[4] = hexToChar(MAC_CONC[0] & 0x0F);
                    wOp->serialBuffer[5] = hexToChar((MAC_CONC[1] >> 4) & 0x0F);
                    wOp->serialBuffer[6] = hexToChar(MAC_CONC[1] & 0x0F);
                    wOp->serialBuffer[7] = hexToChar((MAC_CONC[2] >> 4) & 0x0F);
                    wOp->serialBuffer[8] = hexToChar(MAC_CONC[2] & 0x0F);
                    wOp->serialBuffer[9] = hexToChar((MAC_CONC[3] >> 4) & 0x0F);
                    wOp->serialBuffer[10] = hexToChar(MAC_CONC[3] & 0x0F);
                    wOp->serialBuffer[11] = hexToChar(CONC_TIPO);
                    if(sensorGetConcStatus())
                    {
                      wOp->serialBuffer[12] = '1';
                    }
                    else
                    {
                      wOp->serialBuffer[12] = '0';
                    }
                    
                    //Coloca a posição onde serão armazenados os demais dados
                    wOp->ptSerial = 13;
                  break;
                  
                  case 0x0d:
                    wOp->state = RX_WAIT_RESP;   
                    wOp->ptSerial = 0;
                  break;
                  
                  case '{':
                    wOp->serialBuffer[0] = '{';
                    wOp->state = RX_SENSOR_LIST;
                    wOp->ptSerial = 1;
                    count = 0;
                  break;
                  
                  default:
                  break;
                }
            break;
            
            case RX_MODE_INV:
                if((data != 0x0d) && (wOp->ptSerial < SZ_SERIAL_BUFFER))
                {
                  if(data != ')')
                  {
                    //Armazena os dados no buffer
                    wOp->serialBuffer[wOp->ptSerial++] = data;
                  }
                }
                else
                {
                  //Insere dado de fim de pacote
                  wOp->serialBuffer[wOp->ptSerial++] = '>';
                  
                  //Coloca \0 indicando fim de mensagem
                  wOp->serialBuffer[wOp->ptSerial++] = 0;
                  
                  //Seta flag que indica tipo de mensagem
                  wOp->msgType = W_MSG_INV;

                  //Confirma a recepção
                  wOp->uart->putBuffTx(wOp->uart,'!');
                  
                  //Vai para o estado que espera envio
                  wOp->state = RX_WAIT_SEND;
                }
            break;
            
            case RX_MODE_SEARCH:
                //Grava a mensagem a ser colocada no segundo LCD
                if((data == ']') || (rxLine >= 2))
                {
                  //Confirma a recepção
                  wOp->uart->putBuffTx(wOp->uart,'!');
                  
                  wOp->state = RX_IDLE;
                  
                  if(rxLine == 1)
                  {
                    while(count < 16)
                    {
                      //Preenche o restante da string
                      wOp->serialBuffer[count++] = ' ';
                    }
                  
                    lcdPutStringSearch(wOp->serialBuffer, rxLine, 0, 2);
                    rxLine = 0;
                    count = 0;
                  }
                }
                else if(data == 0x0d)
                {
                  while(count < 16)
                  {
                    //Preenche o restante da string
                    wOp->serialBuffer[count++] = ' ';
                  }
                  
                  lcdPutStringSearch(wOp->serialBuffer, rxLine, 0, 2);
                  rxLine++;
                  count = 0;
                }
                else
                {
                  wOp->serialBuffer[count++] = data;
                }
            break;
            
            case RX_MODE_RX:
                if((data != 0x0d) && (wOp->ptSerial < (SZ_SERIAL_BUFFER - 2)))
                {
                  if(data != '>')
                  {
                    //Armazena os dados no buffer
                    wOp->serialBuffer[wOp->ptSerial++] = data; 
                  }
                }
                else if(data == 0x0d)
                {
                  //Insere dado de fim de pacote
                  wOp->serialBuffer[wOp->ptSerial++] = '>';
                  
                  //Coloca \0 indicando fim de mensagem
                  wOp->serialBuffer[wOp->ptSerial++] = 0;
                 
                  //Verifica a consistência da mensagem
                  if(checkWirelessMessage(wOp->serialBuffer,wOp->ptSerial - 1))
                  {
                      //Seta flag que indica tipo de mensagem
                      wOp->msgType = W_MSG_RX;
                      
                      //Vai para o estado que espera envio
                      wOp->state = RX_WAIT_SEND;
                      
                      //Confirma a recepção
                      wOp->uart->putBuffTx(wOp->uart,'!');
                      
                      //Evita buffer overflow
                      wOp->uart->reset(wOp->uart);  
                  }
                  else
                  {
                     //Considera erro na comunicação
                     wOp->state = RX_IDLE;
                     wOp->ptSerial = 0;
                     wOp->uart->reset(wOp->uart);
                   
                     wOp->uart->putBuffTx(wOp->uart,'@');
                     wOp->uart->putBuffTx(wOp->uart,'@');
                     wOp->uart->putBuffTx(wOp->uart,'@');
                  }
                }
                else
                {
                   //Considera erro na comunicação
                   wOp->state = RX_IDLE;
                   wOp->ptSerial = 0;
                   wOp->uart->reset(wOp->uart);
                   
                   wOp->uart->putBuffTx(wOp->uart,'@');
                   wOp->uart->putBuffTx(wOp->uart,'@');
                   wOp->uart->putBuffTx(wOp->uart,'@');
                }
            break;
            
            case RX_WAIT_RESP:
                if((data != 0x0d) && (wOp->ptSerial < 20))
                {
                  if(data == '<')
                  {
                    //Força ressincronismo
                    wOp->state = RX_MODE_RX;

                    //Coloca cabeçalho da mensagem no buffer
                    wOp->serialBuffer[0] = '<';
                    wOp->serialBuffer[1] = 'W';
                    wOp->serialBuffer[2] = 'R';
                    wOp->serialBuffer[3] = hexToChar((MAC_CONC[0] >> 4) & 0x0F);
                    wOp->serialBuffer[4] = hexToChar(MAC_CONC[0] & 0x0F);
                    wOp->serialBuffer[5] = hexToChar((MAC_CONC[1] >> 4) & 0x0F);
                    wOp->serialBuffer[6] = hexToChar(MAC_CONC[1] & 0x0F);
                    wOp->serialBuffer[7] = hexToChar((MAC_CONC[2] >> 4) & 0x0F);
                    wOp->serialBuffer[8] = hexToChar(MAC_CONC[2] & 0x0F);
                    wOp->serialBuffer[9] = hexToChar((MAC_CONC[3] >> 4) & 0x0F);
                    wOp->serialBuffer[10] = hexToChar(MAC_CONC[3] & 0x0F);
                    wOp->serialBuffer[11] = hexToChar(CONC_TIPO);
                    if(sensorGetConcStatus())
                    {
                      wOp->serialBuffer[12] = '1';
                    }
                    else
                    {
                      wOp->serialBuffer[12] = '0';
                    }
                    
                    //Coloca a posição onde serão armazenados os demais dados
                    wOp->ptSerial = 13;
                  }
                  else
                  {
                      //Armazena os dados no buffer
                      wOp->serialBuffer[wOp->ptSerial++] = data; 
                  }
                }
                else
                {
                  //Coloca \0 indicando fim de mensagem
                  wOp->serialBuffer[wOp->ptSerial++] = 0;
                  
                  //Seta flag que indica tipo de mensagem
                  wOp->msgType = W_MSG_RESP;
                  
                  //Vai para o estado que espera envio
                  wOp->state = RX_WAIT_SEND;
                  
                  //Confirma a recepção
                  wOp->uart->putBuffTx(wOp->uart,'!');
                }
            break;
            
            case RX_SENSOR_LIST:
                if((data != '}') && (wOp->ptSerial < SZ_SERIAL_BUFFER))
                {
                  count++;
                  
                  if((count < 5) && (data != 0x0d) && (data != 0x0a))
                  {
                    //Armazena os dados no buffer
                    wOp->serialBuffer[wOp->ptSerial++] = hexToChar((data >> 4) & 0x0F);
                    wOp->serialBuffer[wOp->ptSerial++] = hexToChar(data & 0x0F);
                  }
                  else
                  {
                    wOp->serialBuffer[wOp->ptSerial++] = data;
                    count = 0;
                  }
                }
                else
                {
                  //Coloca caracteres indicando fim de mensagem
                  wOp->serialBuffer[wOp->ptSerial++] = '}';
                  wOp->serialBuffer[wOp->ptSerial++] = 0;
                  
                  //Seta flag que indica tipo de mensagem
                  wOp->msgType = W_MSG_SENSOR_LIST;
                  
                  //Vai para o estado que espera envio
                  wOp->state = RX_WAIT_SEND;
                  
                  //Confirma a recepção
                  wOp->uart->putBuffTx(wOp->uart,'!');
                }
            break;
            
            case RX_WAIT_SEND:
            break;
            
            default:
              wOp->state = RX_IDLE;
            break;
          }
     }
  }
}