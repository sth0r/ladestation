#include "Macroes.h"

void SPI_MasterInit()
{
	
	// Set MOSI and SCK  & SS output, all others input
	DDRB |= (1<<DDB5)|(1<<DDB3)|(1<<DDB2);// PB5 = SCK, PB3 = MOSI, PB2 = SS(No effect when output)
	//PORTB |= (1<<PORTB2); //Pull up resistor on SS input to ensure Master mode
	// Enable SPI, Master, set clock rate fck/64
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1);
}

void SPI_MasterTransmit(char cData)
{
	// Start transmission
	SPDR = cData;
	// Wait for transmission complete
	while(!(SPSR & (1<<SPIF)));
}

void SPI_SlaveInit(void)
{
	// Set MISO output, all others input
	DDRB |= (1<<DDB4);
	// Set MOSI & SCK & SS input
	DDRB &= ~(1<<DDB5)|~(1<<DDB7)|~(1<<DDB2);
	PORTB &= ~(1<<PORTB2); //Pull down resistor on SS to enable Slave mode
	// Enable SPI
	SPCR = (1<<SPE);
}

char SPI_SlaveReceive(void)
{
	// Wait for reception complete
	while(!(SPSR & (1<<SPIF)));
	// Return data register
	return SPDR;
}