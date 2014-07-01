#ifndef __APP_CONF_H__
#define __APP_CONF_H__

extern volatile const unsigned char * MAC_CONC;

#define SIZE_CONC_MAC 4
#define CONC_TIPO     0x03

#define SIZE_SENS_ID 8
#define SIZE_SENS_TIPO 2
#define SIZE_SENS_VAL 4

#define NUM_SENS 10
#define NUM_ACIONADORES 4
#define NUM_RELES 4

//Mapeamento de memória
#define SIZE_IP 4
#define SIZE_PORTA 2

//original
#define IP_SERVIDOR_0 10
#define IP_SERVIDOR_1 1
#define IP_SERVIDOR_2 1
#define IP_SERVIDOR_3 254

//High Velocity Server
//#define IP_SERVIDOR_0 162
//#define IP_SERVIDOR_1 216
//#define IP_SERVIDOR_2 6
//#define IP_SERVIDOR_3 185

/*
#define IP_SERVIDOR_0 10
#define IP_SERVIDOR_1 10
#define IP_SERVIDOR_2 0
#define IP_SERVIDOR_3 3
*/

#define VERSION_MAJOR '3'
#define VERSION_MINOR '0'

#define PORTA 0x157C  //5500
//#define PORTA 0x1588 //5512
//#define PORTA 0x04D2

#endif