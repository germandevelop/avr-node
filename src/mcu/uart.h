/************************************************************
 *   Author : German Mundinger
 *   Date   : 2020
 ************************************************************/

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stddef.h>

typedef struct uart_config
{
	uint32_t baud_rate;

} uart_config_t;

void uart_init (uart_config_t const * const config);
void uart_deinit ();

void uart_write_byte (uint8_t byte);
void uart_write_byte_array (uint8_t const * const array, size_t array_size);

#ifdef UART_RX_ENABLE
void uart_clean_rx_buffer ();
void uart_read_byte_array (uint8_t *array, size_t array_size, size_t * const bytes_read);
#endif // UART_RX_ENABLE

#endif // UART_H
