/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#include "timer_0.h"

#include <assert.h>

#include <avr/io.h>
#include <avr/interrupt.h>


static timer_0_config_t config;


void timer_0_start_in_ctc_mode (timer_0_config_t const * const init_config)
{
	assert(init_config != NULL);
	assert(init_config->timer_0_callback != NULL);

	config = *init_config;

	// Set the Timer Mode to CTC
	TCCR0A |= (1 << WGM01);

	// Set prescaler
	if (config.prescaler == TIMER_0_PRESCALER_1)
	{
		TCCR0B |= (1 << CS00);
	}
	else if (config.prescaler == TIMER_0_PRESCALER_8)
	{
		TCCR0B |= (1 << CS01);
	}
	else if (config.prescaler == TIMER_0_PRESCALER_64)
	{
		TCCR0B |= (1 << CS01) | (1 << CS00);
	}
	else if (config.prescaler == TIMER_0_PRESCALER_256)
	{
		TCCR0B |= (1 << CS02);
	}
	else if (config.prescaler == TIMER_0_PRESCALER_1024)
	{
		TCCR0B |= (1 << CS02) | (1 << CS00);
	}

	// Set timer tick max value -> throw interruption
	OCR0A = config.prescaler;

	// Set interrupt on compare match
	TIMSK0 |= (1 << OCIE0A);

	return;
}

void timer_0_stop ()
{
	// Disable all interruptions
	TIMSK0 &= ~((1 << OCIE0A) | (1 << OCIE0B) | (1 << TOIE0));

	// Set Timer Mode to Normal
	TCCR0A &= ~((1 << WGM00) | (1 << WGM01));
	TCCR0B &= ~(1 << WGM02);

	// Disable timer
	TCCR0B &= ~((1 << CS00) | (1 << CS01) | (1 << CS02));

	// Reset counter register (counter value)
	TCNT0 = 0U;

	return;
}

ISR (TIMER0_COMPA_vect)
{
	config.timer_0_callback();
}
