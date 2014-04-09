#ifndef _IO_H__
#define _IO_H__

void ioInit(void);
void ioReleSet(unsigned char);
void ioReleClear(unsigned char);
unsigned char ioReleChanged(void);
unsigned char ioReleStatus(unsigned char);

#endif