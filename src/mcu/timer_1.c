/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#include "timer_1.h"

#include <assert.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "mcu/config.h"


static timer_1_config_t config;


void timer_1_start (timer_1_config_t const * const init_config)
{
	assert(init_config != NULL);

	config = *init_config;

	// Setup prescaler
	if (config.prescaler == TIMER_1_PRESCALER_1)
	{
		TCCR1B |= (1 << CS10);
	}
	else if (config.prescaler == TIMER_1_PRESCALER_8)
	{
		TCCR1B |= (1 << CS11);
	}
	else if (config.prescaler == TIMER_1_PRESCALER_64)
	{
		TCCR1B |= (1 << CS11) | (1 << CS10);
	}
	else if (config.prescaler == TIMER_1_PRESCALER_256)
	{
		TCCR1B |= (1 << CS12);
	}
	else if (config.prescaler == TIMER_1_PRESCALER_1024)
	{
		TCCR1B |= (1 << CS12) | (1 << CS10);
	}

	// Setup timer mode
	if (config.mode == TIMER_1_NORMAL_MODE)
	{
		// Do nothing
	}
	else if (config.mode == TIMER_1_COMPARE_MODE)
	{
		TCCR1B |= (1 << WGM12);

		OCR1A	= config.period_a;
		OCR1B	= config.period_b;
	}
	else if (config.mode == TIMER_1_PWM_MODE)
	{
		TCCR1A |= (1 << WGM11);
		TCCR1B |= (1 << WGM13);

		if (config.is_gpio_a_enabled == true)
		{
			TCCR1A |= (1 << COM1A1);
			DDR_TIMER1 |= (1 << PIN_TIMER1_OC1A); 
		}

		if (config.is_gpio_b_enabled == true)
		{
			TCCR1A |= (1 << COM1B1);
			DDR_TIMER1 |= (1 << PIN_TIMER1_OC1B); 
		}

		ICR1	= config.top;
		OCR1A	= config.period_a;
		OCR1B	= config.period_b;
	}

	// Setup interruptions
	if (config.overflow_callback != NULL)
	{
		TIMSK1 |= (1 << TOIE1);
	}
	if (config.compare_a_callback != NULL)
	{
		TIMSK1 |= (1 << OCIE1A);
	}
	if (config.compare_b_callback != NULL)
	{
		TIMSK1 |= (1 << OCIE1B);
	}

	return;
}

void timer_1_stop ()
{
	// Disable all interruptions
	TIMSK1 &= ~((1 << OCIE1A) | (1 << OCIE1B) | (1 << TOIE1) | (1 << ICIE1));

	// Set Timer Mode to Normal
	TCCR1A &= ~((1 << WGM10) | (1 << WGM11));
	TCCR1B &= ~((1 << WGM12) | (1 << WGM13));

	// Disable 'Compare Output Mode' for A and B channels
	TCCR1A &= ~((1 << COM1A0) | (1 << COM1A1) | (1 << COM1B0) | (1 << COM1B1));

	// Disable timer
	TCCR1B &= ~((1 << CS10) | (1 << CS11) | (1 << CS12));

	// Reset counter register (counter value)
	TCNT1 = 0U;

	if (config.is_gpio_a_enabled == true)
	{
		PORT_TIMER1 &= ~(1 << PIN_TIMER1_OC1A);
	}

	if (config.is_gpio_b_enabled == true)
	{
		PORT_TIMER1 &= ~(1 << PIN_TIMER1_OC1B);
	}

	return;
}

ISR (TIMER1_OVF_vect)
{
	config.overflow_callback();
}

ISR (TIMER1_COMPA_vect)
{
	config.compare_a_callback();
}

ISR (TIMER1_COMPB_vect)
{
	config.compare_b_callback();
}
