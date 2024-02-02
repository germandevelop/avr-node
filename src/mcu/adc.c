/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#include "adc.h"

#include <assert.h>

#include <avr/io.h>


void adc_init (adc_config_t const * const config)
{
	assert(config != NULL);

	// Select reference voltage
	if (config->ref_voltage == ADC_REF_VOLTAGE_INTERNAL_1V1)
	{
		ADMUX |= (1 << REFS1) | (1 << REFS0);
	}
	else if (config->ref_voltage == ADC_REF_VOLTAGE_AVCC)
	{
		ADMUX &= ~(1 << REFS1);
		ADMUX |= (1 << REFS0);
	}
	else if (config->ref_voltage == ADC_REF_VOLTAGE_EXTERNAL_AREF)
	{
		ADMUX &= ~((1 << REFS1) | (1 << REFS0));
	}

	// Select input pin
	if (config->input == ADC_INPUT_0)
	{
		ADMUX &= ~((1 << MUX0) | (1 << MUX1) | (1 << MUX2) | (1 << MUX3));
	}
	else if (config->input == ADC_INPUT_1)
	{
		ADMUX &= ~((1 << MUX1) | (1 << MUX2) | (1 << MUX3));
		ADMUX |= (1 << MUX0);
	}
	else if (config->input == ADC_INPUT_2)
	{
		ADMUX &= ~((1 << MUX0) | (1 << MUX2) | (1 << MUX3));
		ADMUX |= (1 << MUX1);
	}
	else if (config->input == ADC_INPUT_3)
	{
		ADMUX &= ~((1 << MUX2) | (1 << MUX3));
		ADMUX |= (1 << MUX0) | (1 << MUX1);
	}
	else if (config->input == ADC_INPUT_4)
	{
		ADMUX &= ~((1 << MUX0) | (1 << MUX1) | (1 << MUX3));
		ADMUX |= (1 << MUX2);
	}
	else if (config->input == ADC_INPUT_5)
	{
		ADMUX &= ~((1 << MUX1) | (1 << MUX3));
		ADMUX |= (1 << MUX0) | (1 << MUX2);
	}

	// Set prescaller to 128 -> best accuracy, but the slowest
	// ADC must be clocked at a frequency between 50 and 200kHz (16MHz/128 = 125KHz)
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

	// Enable ADC
	ADCSRA |= (1 << ADEN);

	return;
}

void adc_deinit ()
{
	// Disable ADC
	ADCSRA &= ~(1 << ADEN);

	// Set external AREF as reference voltage by default
	ADMUX &= ~((1 << REFS1) | (1 << REFS0));

	// Set prescaller to 128 by default -> best accuracy, but the slowest
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

	// Select PC0 as input by default
	ADMUX &= ~((1 << MUX0) | (1 << MUX1) | (1 << MUX2) | (1 << MUX3));

	return;  
}

void adc_read_single_shot (uint16_t * const adc_value)
{
	assert(adc_value != NULL);

	//Start single shot conversion (resets to zero after conversion by itself)
	ADCSRA |= (1 << ADSC);

	// Wait until ADC conversion is complete
	while( ADCSRA & (1 << ADSC) );

	// Read ADC value
	*adc_value = ADC;

	return;
}
