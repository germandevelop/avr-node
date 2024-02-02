/************************************************************
 *   Author : German Mundinger
 *   Date   : 2020
 ************************************************************/

#include "uart.h"

#include <string.h>
#include <assert.h>

#include <avr/io.h>

#include "mcu/config.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


void uart_init (uart_config_t const * const config)
{
	assert(config != NULL);

	// Use 8-bit character sizes
	UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01);

	// Calculate division factor for requested baud rate, and set it
	const uint16_t baudrate_division = (uint16_t)((F_CPU + (config->baud_rate * 8U)) / (config->baud_rate * 16U) - 1U);

	// Setup baudrate
	UBRR0L = (uint8_t)(baudrate_division);
	UBRR0H = (uint8_t)(baudrate_division >> 8);

	// Enable TxD and interrupts
	UCSR0B |= (1 << TXEN0);

#ifdef UART_RX_ENABLE
	// Initialize the buffers
	uart_clean_rx_buffer();
	
	// Enable receiver
	UCSR0B |= (1 << RXCIE0);

	// Enable RxD and interrupts
	UCSR0B |= (1 << RXEN0);
#endif // UART_RX_ENABLE

	return;
}

void uart_deinit ()
{
	// Disable receiver
	UCSR0B &= ~(1 << RXCIE0);

	// Disable RxD/TxD and interrupts
	UCSR0B &= ~((1 << RXEN0) | (1 << TXEN0));

	return;
}

void uart_write_byte (uint8_t byte)
{
	// Wait for the transmitter to be ready
	while ((UCSR0A & (1 << UDRE0)) == 0U);

	// Send byte
	UDR0 = byte;

	return;
}

void uart_write_byte_array (uint8_t const * const array, size_t array_size)
{
	assert(array != NULL);

	for (size_t i = 0U; i < array_size; ++i)
	{
		uart_write_byte(array[i]);
	}
	return;
}


#ifdef UART_RX_ENABLE

#include <avr/interrupt.h>
#include <util/atomic.h>

static uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static volatile size_t rx_buffer_size;

void uart_clean_rx_buffer ()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		rx_buffer_size = 0U;
	}
	return;
}

void uart_read_byte_array (uint8_t *array, size_t array_size, size_t * const bytes_read)
{
	assert(array != NULL);
	assert(bytes_read != NULL);

	*bytes_read = 0U;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (rx_buffer_size != 0U)
		{
			size_t copy_size = array_size;

			if (rx_buffer_size < array_size)
			{
				copy_size = rx_buffer_size;
			}

			memcpy((void*)array, (const void*)rx_buffer, copy_size);
			*bytes_read = copy_size;

			rx_buffer_size = 0U;
		}
	}
	return;
}

ISR (USART_RX_vect)
{
	// Try to avoid buffer overflow -> clear RX buffer
	if (rx_buffer_size == ARRAY_SIZE(rx_buffer))
	{
		rx_buffer_size = 0U;
	}

	// Read byte to RX buffer
	rx_buffer[rx_buffer_size] = UDR0;
	++rx_buffer_size;
}

#endif // UART_RX_ENABLE
