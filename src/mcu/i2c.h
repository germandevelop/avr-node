/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>

typedef struct std_error std_error_t;

typedef enum i2c_frequency
{
	SCL_100KHz = 0,
	SCL_250KHz,
	SCL_400KHz

} i2c_frequency_t;

typedef struct i2c_config
{
	i2c_frequency_t frequency;
	bool is_pullup_enabled;

} i2c_config_t;

void i2c_master_init (i2c_config_t const * const config);
void i2c_master_deinit ();

int i2c_master_write_byte_array (uint8_t slave_device_address,
								uint8_t slave_register_address,
								uint8_t *array,
								uint32_t array_size,
								std_error_t * const error);

int i2c_master_read_byte_array (uint8_t slave_device_address,
								uint8_t slave_register_address,
								uint8_t *array,
								uint32_t array_size,
								std_error_t * const error);

#endif // I2C_H
