#include "utils.h"

/*! \brief Transforma um caracter hexadecimal para ascii */
unsigned char hexToChar(unsigned char hex)
{
  if(hex > 0x0F)
  {
    return '0';
  }
  else if(hex < 0x0A)
  {
    return (hex + '0');
  }
  else
  {
    return (hex - 0x0A + 'A');
  }
}

/*! \brief Transforma dois caracteres em um hexadecimal */
unsigned char charToHex(unsigned char * ch)
{
  unsigned char tmp;
  
  if((ch[0] >= '0') && (ch[0] <= '9'))
  {
    tmp = ((ch[0] - '0') << 4);
  }
  else
  {
    tmp = ((ch[0] - 'A' + 10) << 4);
  }
  
  if((ch[1] >= '0') && (ch[1] <= '9'))
  {
    tmp += (ch[1] - '0');
  }
  else
  {
    tmp += (ch[1] - 'A' + 10);
  }
  
  return tmp;
}

unsigned char binToHex(unsigned char *bin )
{
  unsigned char tmp;
  
  if( (bin[0] == '0') && (bin[1] == '0') && (bin[2] == '0') && (bin[3] == '0') ) tmp = '0';
  if( (bin[0] == '0') && (bin[1] == '0') && (bin[2] == '0') && (bin[3] == '1') ) tmp = '1';
  if( (bin[0] == '0') && (bin[1] == '0') && (bin[2] == '1') && (bin[3] == '0') ) tmp = '2';
  if( (bin[0] == '0') && (bin[1] == '0') && (bin[2] == '1') && (bin[3] == '1') ) tmp = '3';
  if( (bin[0] == '0') && (bin[1] == '1') && (bin[2] == '0') && (bin[3] == '0') ) tmp = '4';
  if( (bin[0] == '0') && (bin[1] == '1') && (bin[2] == '0') && (bin[3] == '1') ) tmp = '5';
  if( (bin[0] == '0') && (bin[1] == '1') && (bin[2] == '1') && (bin[3] == '0') ) tmp = '6';
  if( (bin[0] == '0') && (bin[1] == '1') && (bin[2] == '1') && (bin[3] == '1') ) tmp = '7';
  if( (bin[0] == '1') && (bin[1] == '0') && (bin[2] == '0') && (bin[3] == '0') ) tmp = '8';
  if( (bin[0] == '1') && (bin[1] == '0') && (bin[2] == '0') && (bin[3] == '1') ) tmp = '9';
  if( (bin[0] == '1') && (bin[1] == '0') && (bin[2] == '1') && (bin[3] == '0') ) tmp = 'A';
  if( (bin[0] == '1') && (bin[1] == '0') && (bin[2] == '1') && (bin[3] == '1') ) tmp = 'B';
  if( (bin[0] == '1') && (bin[1] == '1') && (bin[2] == '0') && (bin[3] == '0') ) tmp = 'C';
  if( (bin[0] == '1') && (bin[1] == '1') && (bin[2] == '0') && (bin[3] == '1') ) tmp = 'D';
  if( (bin[0] == '1') && (bin[1] == '1') && (bin[2] == '1') && (bin[3] == '0') ) tmp = 'E';
  if( (bin[0] == '1') && (bin[1] == '1') && (bin[2] == '1') && (bin[3] == '1') ) tmp = 'F';
  
  return tmp;
}

unsigned char checkWirelessMessage(unsigned char * msg, unsigned char size)
{
    static unsigned char i;
    
    for(i=0;i<size;i++)
    {
        //Verifica se é um caracter hexa imprimível
        if(msg[i] < 0x20)
        {
          return 0;
        }
    }
    
    if(size < 13)
    {
        //Tamanho mínimo não atingido
        return 0;
    }
  
    if(size>26)
    {  
        for(i=26;i<=(size-1);i+=14)
        {
            if((msg[i] != ',') && (msg[i] != '>'))
                return 0;
        }
    }
    
    return 1;
}