/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#ifndef SW_UART_H
#define SW_UART_H

#include <stdint.h>
#include <stddef.h>

// A timer interrupt must be set to interrupt at three times the required baud rate.
#define SW_UART_BAUD_RATE 			4800U
#define SW_UART_TIMER_SKIP_COUNT	3U
#define SW_UART_TIMER_PRESCALE		8U
#define SW_UART_TIMER_TICKS			(F_CPU / SW_UART_TIMER_PRESCALE / SW_UART_BAUD_RATE / SW_UART_TIMER_SKIP_COUNT) - 1U

#if (SW_UART_TIMER_TICKS > 255)
    #warning "SW_UART_TIMER_TICKS: increase prescaler, lower F_CPU or use a 16 bit timer"
#endif

void sw_uart_init ();

void sw_uart_write_byte (uint8_t byte);

void sw_uart_write_byte_array (uint8_t const * const array, size_t array_size);

void sw_uart_timer_callback ();

#endif // SW_UART_H

