#ifndef SPI_H
#define SPI_H

// public functions
void initSPI(void);
unsigned int SPIWrite(unsigned char*, unsigned int);
unsigned int SPIRead(unsigned char*, unsigned int);

#endif
