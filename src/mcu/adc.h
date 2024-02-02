/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#ifndef ADC_H
#define ADC_H

#include <stdint.h>

typedef enum adc_input
{
	ADC_INPUT_0 = 0,	// PC0
	ADC_INPUT_1,		// PC1
	ADC_INPUT_2,		// PC2
	ADC_INPUT_3,		// PC3
	ADC_INPUT_4,		// PC4
	ADC_INPUT_5			// PC5

} adc_input_t;

typedef enum adc_ref_voltage
{
	ADC_REF_VOLTAGE_INTERNAL_1V1 = 0,	// internal = 1.1V (with external capacitor at AREF pin)
	ADC_REF_VOLTAGE_AVCC,				// AVCC pin (with external capacitor at AREF pin)
	ADC_REF_VOLTAGE_EXTERNAL_AREF		// AREF pin

} adc_ref_voltage_t;

typedef struct adc_config
{
	adc_input_t input;
	adc_ref_voltage_t ref_voltage;

} adc_config_t;

void adc_init (adc_config_t const * const config);
void adc_deinit ();

void adc_read_single_shot (uint16_t * const adc_value);

#endif // ADC_H
