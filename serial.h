#ifndef _SERIAL_H__
#define _SERIAL_H__

extern int ticks;

void serialInit(void);
unsigned char sensorGetConcStatus(void);
void sensorGetId(unsigned char, void *);
void sensorGetTipo(unsigned char, void *);
void sensorGetValor(unsigned char, void *);
//void sensorGetBatt(unsigned char, void *); //i added this
unsigned char sensorIsSet(unsigned char);
void sensorClear(void);
void sensorSetDataSent(void);

#endif