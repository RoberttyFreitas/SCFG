
#include "spi.h"

#if SPI_CONFIG_AS_MASTER
void spi_init()
{
	SPI_DDR = (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_SS);
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);//prescaler 16
}


uint8_t spi_transmit(uint8_t data)
{
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));
	
	return SPDR;
}

#else
void spi_init()
{
	SPI_DDR = (1<<SPI_MISO);
	SPCR = (1<<SPE);
}

uint8_t spi_transmit(uint8_t data)
{
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}
#endif
