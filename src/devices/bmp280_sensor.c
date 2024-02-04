/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "bmp280_sensor.h"

#include <assert.h>

#include "bmp2.h"

#include "std_error/std_error.h"


#define UNUSED(x) (void)(x)


static bmp280_sensor_config_t config;

static struct bmp2_dev device;
static struct bmp2_config device_config;
static uint32_t delay_ms;


static BMP2_INTF_RET_TYPE bmp280_sensor_read_i2c (uint8_t register_address, uint8_t *array, uint32_t array_size, void *user_data);
static BMP2_INTF_RET_TYPE bmp280_sensor_write_i2c (uint8_t register_address, const uint8_t *array, uint32_t array_size, void *user_data);
static void bmp280_sensor_delay_us (uint32_t period_us, void *user_data);

int bmp280_sensor_init (bmp280_sensor_config_t const * const init_config, std_error_t * const error)
{
    assert(init_config != NULL);
    assert(init_config->read_i2c_callback   != NULL);
    assert(init_config->write_i2c_callback  != NULL);
    assert(init_config->delay_callback      != NULL);

    config = *init_config;

    delay_ms = 4U * 1000U;

    device.intf     = BMP2_I2C_INTF;
    device.read     = bmp280_sensor_read_i2c;
    device.write    = bmp280_sensor_write_i2c;
    device.delay_us = bmp280_sensor_delay_us;
    device.intf_ptr = (void*)error;

    int8_t exit_code = bmp2_init(&device);

    if (exit_code != BMP2_OK)
    {
        return STD_FAILURE;
    }

    exit_code = bmp2_get_config(&device_config, &device);

    if (exit_code != BMP2_OK)
    {
        return STD_FAILURE;
    }

    device_config.os_temp   = BMP2_OS_2X;
    device_config.os_pres   = BMP2_OS_16X;
    device_config.filter    = BMP2_FILTER_COEFF_16;
    device_config.os_mode   = BMP2_OS_MODE_HIGH_RESOLUTION;

    exit_code = bmp2_set_config(&device_config, &device);

    if (exit_code != BMP2_OK)
    {
        return STD_FAILURE;
    }

    exit_code = bmp2_compute_meas_time(&delay_ms, &device_config, &device);

    if (exit_code == BMP2_OK)
    {
        delay_ms = (delay_ms / 1000U) + 100U;
    }
    else
    {
        delay_ms = 4U * 1000U;
    }

    exit_code = bmp2_set_power_mode(BMP2_POWERMODE_SLEEP, &device_config, &device);

    if (exit_code != BMP2_OK)
    {
        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

int bmp280_sensor_read_data (bmp280_sensor_data_t * const data, std_error_t * const error)
{
    device.intf_ptr = (void*)error;

    int8_t exit_code = bmp2_set_power_mode(BMP2_POWERMODE_FORCED, &device_config, &device);

    if (exit_code != BMP2_OK)
    {
        return STD_FAILURE;
    }

    config.delay_callback(delay_ms);

    struct bmp2_data sensor_data;
    exit_code = bmp2_get_sensor_data(&sensor_data, &device);

    if (exit_code != BMP2_OK)
    {
        return STD_FAILURE;
    }

    data->temperature_C = (float)sensor_data.temperature;
    data->pressure_hPa  = (float)(0.01 * sensor_data.pressure);

    return STD_SUCCESS;
}


BMP2_INTF_RET_TYPE bmp280_sensor_read_i2c (uint8_t register_address, uint8_t *array, uint32_t array_size, void *user_data)
{
    std_error_t *error = (std_error_t*)user_data;

    return config.read_i2c_callback(BMP2_I2C_ADDR_PRIM, register_address, array, array_size, error);
}

BMP2_INTF_RET_TYPE bmp280_sensor_write_i2c (uint8_t register_address, const uint8_t *array, uint32_t array_size, void *user_data)
{
    std_error_t *error = (std_error_t*)user_data;

    return config.write_i2c_callback(BMP2_I2C_ADDR_PRIM, register_address, (uint8_t*)array, array_size, error);
}

void bmp280_sensor_delay_us (uint32_t period_us, void *user_data)
{
    UNUSED(user_data);

    config.delay_callback((period_us / 1000U) + 1U);

    return;
}
