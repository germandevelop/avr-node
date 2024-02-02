/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#ifndef TIMER_0_H
#define TIMER_0_H

#include <stdint.h>

typedef void (*timer_0_callback_t)();

typedef enum timer_0_prescaler
{
	TIMER_0_PRESCALER_1 = 0,
	TIMER_0_PRESCALER_8,
	TIMER_0_PRESCALER_64,
	TIMER_0_PRESCALER_256,
	TIMER_0_PRESCALER_1024

} timer_0_prescaler_t;

typedef struct timer_0_config
{
	timer_0_callback_t timer_0_callback;
	timer_0_prescaler_t prescaler;
	uint8_t ticks;

} timer_0_config_t;

void timer_0_start_in_ctc_mode (timer_0_config_t const * const init_config);

void timer_0_stop ();

#endif // TIMER_0_H
