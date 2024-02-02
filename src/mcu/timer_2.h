/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#ifndef TIMER_2_H
#define TIMER_2_H

#include <stdint.h>

typedef void (*timer_2_callback_t)();

typedef enum timer_2_prescaler
{
	TIMER_2_PRESCALER_1 = 0,
	TIMER_2_PRESCALER_8,
	TIMER_2_PRESCALER_32,
	TIMER_2_PRESCALER_64,
	TIMER_2_PRESCALER_128,
	TIMER_2_PRESCALER_256,
	TIMER_2_PRESCALER_1024

} timer_2_prescaler_t;

typedef struct timer_2_config
{
	timer_2_callback_t timer_2_callback;
	timer_2_prescaler_t prescaler;
	uint8_t ticks;

} timer_2_config_t;

void timer_2_start_in_ctc_mode (timer_2_config_t const * const init_config);

void timer_2_stop ();

#endif // TIMER_2_H
