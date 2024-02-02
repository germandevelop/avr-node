/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#include "spi.h"

#include <assert.h>

#include <avr/io.h>

#include "mcu/config.h"


void spi_master_init (spi_config_t const * const config)
{
	assert(config != NULL);

	// Set MOSI, SCK and SS to output
	DDR_SPI |= (1 << PIN_MOSI) | (1 << PIN_SCK) | (1 << PIN_SS);

	// Set MOSI and SCK to low
	PORT_SPI &= ~((1 << PIN_MOSI) | (1 << PIN_SCK));

	// Set MISO to input
	DDR_SPI &= ~(1 << PIN_MISO);

	// Disable MISO pullup
	PORT_SPI &= ~(1 << PIN_MISO);

	// Set SPI clock frequency
	if (config->clock_rate == CPU_FREQUENCY_DIVIDED_BY_2)
	{
		SPCR &= ~((1 << SPR1) | (1 << SPR0));
		SPSR |= (1 << SPI2X);
	}
	else if (config->clock_rate == CPU_FREQUENCY_DIVIDED_BY_4)
	{
		SPCR &= ~((1 << SPR1) | (1 << SPR0));
		SPSR &= ~(1 << SPI2X);
	}
	else if (config->clock_rate == CPU_FREQUENCY_DIVIDED_BY_8)
	{
		SPCR |= (1 << SPR0);
		SPCR &= ~(1 << SPR1);
		SPSR |= (1 << SPI2X);
	}
	else if (config->clock_rate == CPU_FREQUENCY_DIVIDED_BY_16)
	{
		SPCR |= (1 << SPR0);
		SPCR &= ~(1 << SPR1);
		SPSR &= ~(1 << SPI2X);
	}
	else if (config->clock_rate == CPU_FREQUENCY_DIVIDED_BY_32)
	{
		SPCR &= ~(1 << SPR0);
		SPCR |= (1 << SPR1);
		SPSR |= (1 << SPI2X);
	}
	else if (config->clock_rate == CPU_FREQUENCY_DIVIDED_BY_64)
	{
		SPCR &= ~(1 << SPR0);
		SPCR |= (1 << SPR1);
		SPSR &= ~(1 << SPI2X);
	}
	else if (config->clock_rate == CPU_FREQUENCY_DIVIDED_BY_128)
	{
		SPCR |= ((1 << SPR1) | (1 << SPR0));
		SPSR &= ~(1 << SPI2X);
	}

	// Set as master
	SPCR |= (1 << MSTR);

	// Enable SPI
	SPCR |= (1 << SPE);

	return;
}

void spi_master_deinit ()
{
	// Set inital register state (zero)
	SPSR &= ~(1 << SPI2X);

	// Set inital register state (all zeros)
	SPCR &= ~((1 << SPIE) | (1 << SPE) | (1 << DORD) | (1 << MSTR) | (1 << CPOL) | (1 << CPHA) | (1 << SPR1) | (1 << SPR0));

	// Set all pins to input
	DDR_SPI &= ~((1 << PIN_MOSI) | (1 << PIN_MISO) | (1 << PIN_SCK));

	return;
}

void spi_master_write_byte (uint8_t byte)
{
	// Load dummy data into register
	SPDR = byte;

	// Wait for transmission complete
	while ((SPSR & (1 << SPIF)) == 0U);

	return;
}

void spi_master_read_byte (uint8_t * const byte)
{
	assert(byte != NULL);

	// Load data into register
	SPDR = 0xFF;

	// Wait for transmission complete
	while ((SPSR & (1 << SPIF)) == 0U);

	// Read byte from register
	*byte = SPDR;

	return;
}
