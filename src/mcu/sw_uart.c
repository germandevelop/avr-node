/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#include "sw_uart.h"

#include <stdbool.h>

#include <avr/io.h>

#include "mcu/config.h"

// Software UART
#define SW_UART_TX_DDR  DDRD
#define SW_UART_TX_PORT PORTD
#define SW_UART_TX_BIT  PORTD2

static volatile uint8_t tx_buffer;
static volatile bool is_tx_working;

void sw_uart_init ()
{
	tx_buffer 		= 0U;
	is_tx_working 	= false;

	// Set TX-Pin to high to avoid garbage on init 
	SW_UART_TX_PORT |= (1 << SW_UART_TX_BIT);

	// TX-Pin as output
	SW_UART_TX_DDR |= (1 << SW_UART_TX_BIT);

	return;
}

void sw_uart_write_byte (uint8_t byte)
{
	// Wait for transmitter ready
	// Add watchdog-reset here if needed
	while (is_tx_working == true);

	// Start TX transmitting
	tx_buffer 		= byte;
	is_tx_working 	= true;

	return;
}

void sw_uart_write_byte_array (uint8_t const * const array, size_t array_size)
{
	for (size_t i = 0U; i < array_size; ++i)
	{
		sw_uart_write_byte(array[i]);
	}
	return;
}

void sw_uart_timer_callback ()
{
	// 1 start bit, 8 data bits, 1 stop bit = 10 bits/frame, skip 2 timer interrupts
	static bool is_start_bit		= true;
	static bool is_stop_bit			= false;
	static size_t data_bit_count	= 0U;
	static size_t skip_count		= 0U;

	// Transmitter section
	if (is_tx_working == true)
	{
		// Skip 2 timer interruptions
		if (skip_count < (SW_UART_TIMER_SKIP_COUNT - 1U))
		{
			++skip_count;

			return;
		}
		else
		{
			skip_count = 0U;
		}
		
		bool is_tx_pin_high = false;

		// Write 'start bit' - 0
		if (is_start_bit == true)
		{
			is_tx_pin_high	= false;

			is_start_bit	= false;
			is_stop_bit		= false;
			data_bit_count	= 0U;
		}

		// Write 'stop bit' - 1
		else if (is_stop_bit == true)
		{
			is_tx_pin_high	= true;

			is_start_bit	= true;
			is_stop_bit		= false;

			is_tx_working 	= false;
		}

		// Write 'data bit'
		else
		{
			const uint8_t tx_bit = (tx_buffer >> data_bit_count) & 1U;

			if (tx_bit > 0U)
			{
				is_tx_pin_high = true;
			}
			else
			{
				is_tx_pin_high = false;
			}
	
			++data_bit_count;

			if (data_bit_count > 7U)
			{
				is_stop_bit = true;
			}
		}

		// Set TX-pin state
		// Set TX-pin to high
		if (is_tx_pin_high == true)
		{
			SW_UART_TX_PORT |= (1 << SW_UART_TX_BIT);
		}

		// Set TX-pin to low
		else
		{
			SW_UART_TX_PORT &= ~(1 << SW_UART_TX_BIT);
		}
	}
	return;
}

