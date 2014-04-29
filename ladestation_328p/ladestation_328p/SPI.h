
#ifndef SPI_H_
#define SPI_H_

extern void SPI_MasterInit();
extern void SPI_MasterTransmit(char cData);
extern void SPI_SlaveInit(void);
extern char SPI_SlaveReceive(void);

#endif /* SPI_H_ */