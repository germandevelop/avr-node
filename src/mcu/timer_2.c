/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#include "timer_2.h"

#include <assert.h>

#include <avr/io.h>
#include <avr/interrupt.h>


static timer_2_config_t config;


void timer_2_start_in_ctc_mode (timer_2_config_t const * const init_config)
{
	assert(init_config != NULL);
	assert(init_config->timer_2_callback != NULL);

	config = *init_config;

	// Set the Timer Mode to CTC
	TCCR2A |= (1 << WGM21);

	// Set prescaler
	if (config.prescaler == TIMER_2_PRESCALER_1)
	{
		TCCR2B |= (1 << CS20);
	}
	else if (config.prescaler == TIMER_2_PRESCALER_8)
	{
		TCCR2B |= (1 << CS21);
	}
	else if (config.prescaler == TIMER_2_PRESCALER_32)
	{
		TCCR2B |= (1 << CS21) | (1 << CS20);
	}
	else if (config.prescaler == TIMER_2_PRESCALER_64)
	{
		TCCR2B |= (1 << CS22);
	}
	else if (config.prescaler == TIMER_2_PRESCALER_128)
	{
		TCCR2B |= (1 << CS22) | (1 << CS20);
	}
	else if (config.prescaler == TIMER_2_PRESCALER_256)
	{
		TCCR2B |= (1 << CS22) | (1 << CS21);
	}
	else if (config.prescaler == TIMER_2_PRESCALER_1024)
	{
		TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);
	}

	// Set timer tick max value -> throw interruption
	OCR2A = config.ticks;

	// Set interrupt on compare match
	TIMSK2 |= (1 << OCIE2A);

	return;
}

void timer_2_stop ()
{
	// Disable all interruptions
	TIMSK2 &= ~((1 << OCIE2A) | (1 << OCIE2B) | (1 << TOIE2));

	// Set Timer Mode to Normal
	TCCR2A &= ~((1 << WGM20) | (1 << WGM21));

	// Disable timer
	TCCR2B &= ~((1 << CS20) | (1 << CS21) | (1 << CS22));

	// Reset counter register (counter value)
	TCNT2 = 0U;

	return;
}

ISR (TIMER2_COMPA_vect)
{
	config.timer_2_callback();
}
