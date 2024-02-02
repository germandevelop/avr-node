/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#ifndef TIMER_1_H
#define TIMER_1_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*timer_1_callback_t)();

typedef enum timer_1_mode
{
	TIMER_1_NORMAL_MODE = 0,
	TIMER_1_COMPARE_MODE,
	TIMER_1_PWM_MODE

} timer_1_mode_t;

typedef enum timer_1_prescaler
{
	TIMER_1_PRESCALER_1 = 0,
	TIMER_1_PRESCALER_8,
	TIMER_1_PRESCALER_64,
	TIMER_1_PRESCALER_256,
	TIMER_1_PRESCALER_1024

} timer_1_prescaler_t;

typedef struct timer_1_config
{
	// Normal Mode
	timer_1_mode_t mode;
	timer_1_prescaler_t prescaler;
	timer_1_callback_t overflow_callback; // May be NULL (Pointless for Compare mode)

	// Compare output mode (channel A and B)
	uint16_t period_a;
	uint16_t period_b;
	timer_1_callback_t compare_a_callback; // May be NULL
	timer_1_callback_t compare_b_callback; // May be NULL

	// PWM phase correct mode
	uint16_t top;
	bool is_gpio_a_enabled;
	bool is_gpio_b_enabled;

} timer_1_config_t;

void timer_1_start (timer_1_config_t const * const init_config);
void timer_1_stop ();

#endif // TIMER_1_H
