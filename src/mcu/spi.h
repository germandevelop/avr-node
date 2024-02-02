/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#ifndef SPI_H
#define SPI_H

#include <stdint.h>

typedef enum spi_clock_rate
{
	CPU_FREQUENCY_DIVIDED_BY_2 = 0, // Fosc / 2 (max speed)
	CPU_FREQUENCY_DIVIDED_BY_4,		// Fosc / 4
	CPU_FREQUENCY_DIVIDED_BY_8,		// Fosc / 8
	CPU_FREQUENCY_DIVIDED_BY_16,	// Fosc / 16
	CPU_FREQUENCY_DIVIDED_BY_32,	// Fosc / 32
	CPU_FREQUENCY_DIVIDED_BY_64,	// Fosc / 64
	CPU_FREQUENCY_DIVIDED_BY_128	// Fosc / 128

} spi_clock_rate_t;

typedef struct spi_config
{
	spi_clock_rate_t clock_rate;

} spi_config_t;

void spi_master_init (spi_config_t const * const config);
void spi_master_deinit ();

void spi_master_write_byte (uint8_t byte);
void spi_master_read_byte (uint8_t * const byte);

#endif // SPI_H
