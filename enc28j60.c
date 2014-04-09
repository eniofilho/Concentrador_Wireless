#include <intrinsics.h>
#include "io430.h"
#include "enc28j60.h"
#include "uip.h"
#include "uipopt.h"

unsigned char Enc28j60Bank;
unsigned int NextPacketPtr;

void delay_us(unsigned long x)
{
  unsigned long a;
  for(a=0;a<(x);a++)
   asm("nop");
}

void assertCS(void) {
	ENC_SPI_POUT &= ~(1<<ENC_CS);			// assert CS
}

void releaseCS(void) {
	ENC_SPI_POUT |= (1<<ENC_CS);				// release CS
}

void init_spi(void) {
	
        // Configure I/O pins	
	ENC_SPI_DIR  |= (1 << ENC_CS);
	ENC_SPI_POUT |= (1 << ENC_CS);
	ENC_SPI_DIR |= (1 << ENC_SCK) + (1 << ENC_MOSI) + (1 << ENC_RESET);
	ENC_SPI_DIR &= ~(1 << ENC_MISO);
        ENC_SPI_DIR &= ~(1 << ENC_INT);
	ENC_SPI_SEL |= (1 << ENC_SCK) + (1 << ENC_MOSI) + (1 << ENC_MISO);
        ENC_SPI_SEL &= ~(1 << ENC_RESET);
        ENC_SPI_SEL &= ~(1 << ENC_INT);

        // Set SPI registers
        UCB0CTL1 |= UCSSEL_2;
        UCB0CTL0 |= UCMST + UCSYNC + UCMSB + UCCKPH;
        UCB0BR0 = 3;
        UCB0BR1 = 0;
        UCB0CTL1 &= ~UCSWRST;

        //Reset
        ENC_SPI_POUT |= (1 << ENC_RESET);
        delay_us(30);
        ENC_SPI_POUT &= ~(1 << ENC_RESET);
        delay_us(30);
        ENC_SPI_POUT |= (1 << ENC_RESET);
        delay_us(30);
}

#pragma inline
unsigned char spi_rw_byte(unsigned char data) {
	//while((IFG2 & UCB0TXIFG) == 0);    	// wait until TX buffer empty
	UCB0TXBUF = data;						// send byte
  	while((IFG2 & UCB0RXIFG) == 0) {}		// data present in RX buffer?
	return UCB0RXBUF;						// return read data
}

unsigned char enc28j60ReadOp(unsigned char op, unsigned char address) {
	unsigned char data = 0;
	assertCS();							// assert CS signal

	spi_rw_byte(op | (address & ADDR_MASK));	// issue read command
	data = spi_rw_byte(0);				// read data (send zeroes)
	
	if(address & 0x80) {				// do dummy read if needed
		data = spi_rw_byte(0);			// read data (send zeroes)
	}

	releaseCS();						// release CS signal
	return data;
}

void enc28j60WriteOp(unsigned char op, unsigned char address, unsigned char data) {
	assertCS();
	spi_rw_byte(op | (address & ADDR_MASK));// issue write command
	if (op != ENC28J60_SOFT_RESET) spi_rw_byte(data);	// send data
    releaseCS();
}


void enc28j60ReadBuffer(unsigned int len, unsigned char* data) {
	assertCS();
	
	spi_rw_byte(ENC28J60_READ_BUF_MEM);		// issue read command
	while(len--) {
		*data++ = spi_rw_byte(0);
	}
	releaseCS();
}


void enc28j60WriteBuffer(unsigned int len, unsigned char* data) {
	assertCS();

	spi_rw_byte(ENC28J60_WRITE_BUF_MEM);		// issue write command
	while(len--) {
		spi_rw_byte(*data++);
	}
	releaseCS();
}

void enc28j60SetBank(unsigned char address) {
	if((address & BANK_MASK) != Enc28j60Bank) {	// set the bank (if needed)
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
		Enc28j60Bank = (address & BANK_MASK);
	}
}

unsigned char enc28j60Read(unsigned char address) {
	enc28j60SetBank(address);									// set the bank
	return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);		// do the read
}


void enc28j60Write(unsigned char address, unsigned char data) {
	enc28j60SetBank(address);									// set the bank
	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);	// do the write
}

void enc28j60PhyWrite(unsigned char address, unsigned int data) {
	enc28j60Write(MIREGADR, address);		// set the PHY register address
	enc28j60Write(MIWRL, data);				// write the PHY data
	enc28j60Write(MIWRH, data>>8);

	while(enc28j60Read(MISTAT) & MISTAT_BUSY);	// wait until the PHY write completes
}

unsigned int enc28j60PhyRead(unsigned char address)
{
	unsigned int data;

	// Set the right address and start the register read operation
	enc28j60Write(MIREGADR, address);
	enc28j60Write(MICMD, MICMD_MIIRD);

	// wait until the PHY read completes
	while(enc28j60Read(MISTAT) & MISTAT_BUSY);

	// quit reading
	enc28j60Write(MICMD, 0x00);
	
	// get data value
	data  = enc28j60Read(MIRDL);
	data |= enc28j60Read(MIRDH);

	// return the data
	return data;
}

void enc28j60_init(void) {
	init_spi();
	// set clock to 8.333MHz
	//enc28j60Write(ECOCON, 0x03);

        // check CLKRDY bit to see if reset is complete
	while(!(enc28j60Read(ESTAT) & ESTAT_CLKRDY));

	// perform system reset
	enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
        delay_us(10000);
        delay_us(10000);
        delay_us(10000);
        delay_us(10000);
        delay_us(10000);
        delay_us(10000);
        delay_us(10000);
        delay_us(10000);
        delay_us(10000);
        delay_us(10000);
        delay_us(10000);

	// check CLKRDY bit to see if reset is complete
	while(!(enc28j60Read(ESTAT) & ESTAT_CLKRDY));
	
	// Set LED configuration	
	//enc28j60PhyWrite(PHLCON, 0x3ba6);
	enc28j60PhyWrite(PHLCON,0x3212);

	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	NextPacketPtr = RXSTART_INIT;
	enc28j60Write(ERXSTL, RXSTART_INIT&0xFF);
	enc28j60Write(ERXSTH, RXSTART_INIT>>8);
	// set receive pointer address
	enc28j60Write(ERXRDPTL, RXSTART_INIT&0xFF);
	enc28j60Write(ERXRDPTH, RXSTART_INIT>>8);
	// set receive buffer end
	// ERXND defaults to 0x1FFF (end of ram)
	enc28j60Write(ERXNDL, RXSTOP_INIT&0xFF);
	enc28j60Write(ERXNDH, RXSTOP_INIT>>8);
	// set transmit buffer start
	// ETXST defaults to 0x0000 (beginnging of ram)
	enc28j60Write(ETXSTL, TXSTART_INIT&0xFF);
	enc28j60Write(ETXSTH, TXSTART_INIT>>8);

	// do bank 2 stuff
	// enable MAC receive
	enc28j60Write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// enable automatic padding and CRC operations
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
	// set MACON 4 bits (necessary for half-duplex only)
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON4, MACON4_DEFER);

	// set inter-frame gap (non-back-to-back)
	enc28j60Write(MAIPGL, 0x12);
	enc28j60Write(MAIPGH, 0x0C);
	// set inter-frame gap (back-to-back)
	enc28j60Write(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
	enc28j60Write(MAMXFLL, MAX_FRAMELEN&0xFF);
	enc28j60Write(MAMXFLH, MAX_FRAMELEN>>8);

	// do bank 3 stuff
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	enc28j60Write(MAADR6, MAC_CONC[3]);
	enc28j60Write(MAADR5, MAC_CONC[2]);
	enc28j60Write(MAADR4, MAC_CONC[1]);
	enc28j60Write(MAADR3, MAC_CONC[0]);
	enc28j60Write(MAADR2, ENC28J60_MAC1);
	enc28j60Write(MAADR1, ENC28J60_MAC0);
	
	// no loopback of transmitted frames
	enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);

	// switch to bank 0
	enc28j60SetBank(ECON1);

        //Filtra por MAC
        enc28j60Write(ERXFCON,0x81);
        //enc28j60Write(ERXFCON,0);

        //Habilita interrupção
        enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_PKTIE | EIE_INTIE);

	// enable packet reception
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
}


void enc28j60PacketSend(unsigned int len, unsigned char* packet) {
	// Set the write pointer to start of transmit buffer area
	enc28j60Write(EWRPTL, TXSTART_INIT);
	enc28j60Write(EWRPTH, TXSTART_INIT>>8);
	// Set the TXND pointer to correspond to the packet size given
	enc28j60Write(ETXNDL, (TXSTART_INIT+len));
	enc28j60Write(ETXNDH, (TXSTART_INIT+len)>>8);

	// write per-packet control byte
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

	// copy the packet into the transmit buffer
	enc28j60WriteBuffer(len, packet);

	// send the contents of the transmit buffer onto the network
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

unsigned int enc28j60PacketReceive(unsigned int maxlen, unsigned char* packet) {
	unsigned int rxstat;
	unsigned int len;

	// check if a packet has been received and buffered
	//if(!enc28j60Read(EPKTCNT)) return 0;
        if(ENC_SPI_PIN & ENC_INT_BIT) return 0;

	// Set the read pointer to the start of the received packet
	enc28j60Write(ERDPTL, (NextPacketPtr));
	enc28j60Write(ERDPTH, (NextPacketPtr)>>8);
	// read the next packet pointer
	NextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	NextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// read the packet length
	len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// read the receive status
	rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;

	// limit retrieve length
	// (we reduce the MAC-reported length by 4 to remove the CRC)
	len = MIN(len, maxlen);

	// copy the packet from the receive buffer
	enc28j60ReadBuffer(len, packet);

	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
	enc28j60Write(ERXRDPTL, (NextPacketPtr));
	enc28j60Write(ERXRDPTH, (NextPacketPtr)>>8);

	// decrement the packet counter indicate we are done with this packet
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);

	return len;
}

// For compatibility with the CS8900A package
void enc28j60_send(void) {
	enc28j60PacketSend(uip_len, uip_buf);
}

// For compatibility with the CS8900A package
u16_t enc28j60_poll(void) {
	return enc28j60PacketReceive(UIP_BUFSIZE, uip_buf);
}

// This function has to be defined by the application - ohterwise MSPGCC doesn't compile
// This function is never used by telnetd
int putchar(int c){return 0;}
