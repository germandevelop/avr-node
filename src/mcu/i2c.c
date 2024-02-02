/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#include "i2c.h"

#include <assert.h>

#include <avr/io.h>
#include <util/twi.h>

#include "mcu/config.h"

#include "std_error/std_error.h"


#define FILE_NAME               "i2c.c"
#define I2C_DEFAULT_ERROR_TEXT  "I2C error"


static int i2c_master_start (std_error_t * const error);
static int i2c_master_write_slave_address_for_writing (uint8_t slave_address, std_error_t * const error);
static int i2c_master_write_slave_address_for_reading (uint8_t slave_address, std_error_t * const error);
static int i2c_master_write_byte (uint8_t byte, std_error_t * const error);
static int i2c_master_read_byte_with_acknowledgement (uint8_t * const byte, std_error_t * const error);
static int i2c_master_read_byte_without_acknowledgement (uint8_t * const byte, std_error_t * const error);
static void i2c_master_stop ();

void i2c_master_init (i2c_config_t const * const config)
{
    assert(config != NULL);

    // Try to set input pull-up resistor
    DDR_I2C |= (1 << PIN_I2C_SDA) | (1 << PIN_I2C_SCL); // Set SDA and SCL to output

    if (config->is_pullup_enabled == true)
    {
        PORT_I2C |= (1 << PIN_I2C_SDA) | (1 << PIN_I2C_SCL); // Set SDA and SCL to high
    }
    else
    {
        PORT_I2C &= ~((1 << PIN_I2C_SDA) | (1 << PIN_I2C_SCL)); // Set SDA and SCL to low
    }
    DDR_I2C &= ~((1 << PIN_I2C_SDA) | (1 << PIN_I2C_SCL)); // Set SDA and SCL back to input

    // Setup I2C SCL freuency
    // SCL frequency = CPU clock frequency / (16 + 2 * TWBR * PrescalerValue)
    uint8_t bit_rate_register_values[] =
    {
        [SCL_100KHz] = 72U, // Set bit rate register 72 and prescaler to 1 resulting in SCL_freq = 16MHz/(16 + 2*72*1) = 100KHz
        [SCL_250KHz] = 24U, // Set bit rate register 24 and prescaler to 1 resulting in SCL_freq = 16MHz/(16 + 2*24*1) = 250KHz
        [SCL_400KHz] = 12U  // Set bit rate register 12 and prescaler to 1 resulting in SCL_freq = 16MHz/(16 + 2*12*1) = 400KHz
    };

    TWSR &= ~((1 << TWPS0) | (1 << TWPS1)); // Set prescaler to 1
    TWBR = bit_rate_register_values[config->frequency];

    return;
}

void i2c_master_deinit ()
{
    // Set inital register state (zero)
    TWBR = 0x0;

    // Set inital register state
    TWSR |= (1 << TWPS0) | (1 << TWPS1); // Default prescaler value is 64

    // Set all pins to input
    DDR_I2C &= ~((1 << PIN_I2C_SDA) | (1 << PIN_I2C_SCL));

    return;
}

int i2c_master_write_byte_array (uint8_t slave_device_address,
                                uint8_t slave_register_address,
                                uint8_t *array,
                                uint32_t array_size,
                                std_error_t * const error)
{
    assert(array != NULL);

    // Try to lock I2C bus for master
    int ret_val = i2c_master_start(error);

    if (ret_val != STD_FAILURE)
    {
        // Try to select slave device and start communication with it
        ret_val = i2c_master_write_slave_address_for_writing(slave_device_address, error);

        if (ret_val != STD_FAILURE)
        {
            // Try to send register address to slave device
            ret_val = i2c_master_write_byte(slave_register_address, error);

            if (ret_val != STD_FAILURE)
            {
                // Try to send data to the register
                for (size_t i = 0U; i < array_size; ++i)
                {
                    ret_val = i2c_master_write_byte(array[i], error);

                    if (ret_val != STD_SUCCESS)
                    {
                        break;
                    }
                }
            }
        }
        // Unlock I2C bus
        i2c_master_stop();
    }
    return ret_val;
}

int i2c_master_read_byte_array (uint8_t slave_device_address,
                                uint8_t slave_register_address,
                                uint8_t *array,
                                uint32_t array_size,
                                std_error_t * const error)
{
    assert(array != NULL);
    
    // Try to lock I2C bus for master
    int ret_val = i2c_master_start(error);

    if (ret_val != STD_FAILURE)
    {
        // Try to select slave device and start communication with it
        ret_val = i2c_master_write_slave_address_for_writing(slave_device_address, error);

        if (ret_val != STD_FAILURE)
        {
            // Try to send register address to slave device
            ret_val = i2c_master_write_byte(slave_register_address, error);

            if (ret_val != STD_FAILURE)
            {
                // Try to relock I2C bus again for reading
                ret_val = i2c_master_start(error);

                if (ret_val != STD_FAILURE)
                {
                    // Try to switch slave device communication mode
                    ret_val = i2c_master_write_slave_address_for_reading(slave_device_address, error);

                    if (ret_val != STD_FAILURE)
                    {
                        // Try to read data from the register
                        for (size_t i = 0U; i < array_size; ++i)
                        {
                            if (i < (array_size - 1U))
                            {
                                ret_val = i2c_master_read_byte_with_acknowledgement(&array[i], error); // Each byte with ACK
                            }
                            else
                            {
                                ret_val = i2c_master_read_byte_without_acknowledgement(&array[i], error); // Last byte without ACK
                            }

                            if (ret_val != STD_SUCCESS)
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }
        // Unlock I2C bus
        i2c_master_stop();
    }
    return ret_val;
}

int i2c_master_start (std_error_t * const error)
{
    // Configure Control register
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);

    // Wait for TWINT flag to set
    while ((TWCR & (1 << TWINT)) == 0U);

    // Check errors
    // Start condition transmitted
    if ((TW_STATUS != TW_START) && (TW_STATUS != TW_REP_START))
    {
        std_error_catch_custom(error, (int)TW_STATUS, I2C_DEFAULT_ERROR_TEXT, FILE_NAME, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

void i2c_master_stop ()
{
    // Configure Control register
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);

    return;
}

int i2c_master_write_slave_address_for_writing (uint8_t slave_address, std_error_t * const error)
{
    // SLA+W address
    TWDR = ((slave_address << 1) | TW_WRITE);

    // Configure Control register
    TWCR = (1 << TWINT) | (1 << TWEN);

    // Wait for TWINT flag to set
    while ((TWCR & (1 << TWINT)) == 0U);

    // Check errors
    // SLA+W transmitted, ACK received
    if (TW_STATUS != TW_MT_SLA_ACK)
    {
        std_error_catch_custom(error, (int)TW_STATUS, I2C_DEFAULT_ERROR_TEXT, FILE_NAME, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

int i2c_master_write_slave_address_for_reading (uint8_t slave_address, std_error_t * const error)
{
    // SLA+R address
    TWDR = ((slave_address << 1) | TW_READ);

    // Configure Control register
    TWCR = (1 << TWINT) | (1 << TWEN);

    // Wait for TWINT flag to set
    while ((TWCR & (1 << TWINT)) == 0U);

    // Check errors
    // SLA+R transmitted, ACK received
    if (TW_STATUS != TW_MR_SLA_ACK)
    {
        std_error_catch_custom(error, (int)TW_STATUS, I2C_DEFAULT_ERROR_TEXT, FILE_NAME, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

int i2c_master_write_byte (uint8_t byte, std_error_t * const error)
{
    // Store byte to data register
    TWDR = byte;

    // Configure Control register
    TWCR = (1 << TWINT) | (1 << TWEN);

    // Wait for TWINT flag to set
    while ((TWCR & (1 << TWINT)) == 0U);

    // Check errors
    // Data transmitted, ACK received
    if (TW_STATUS != TW_MT_DATA_ACK)
    {
        std_error_catch_custom(error, (int)TW_STATUS, I2C_DEFAULT_ERROR_TEXT, FILE_NAME, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

int i2c_master_read_byte_with_acknowledgement (uint8_t * const byte, std_error_t * const error)
{
    // Configure Control register
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    // Wait for TWINT flag to set
    while ((TWCR & (1 << TWINT)) == 0U);

    // Read byte from data register
    *byte = TWDR;

    // Check errors
    // Data received, ACK returned
    if (TW_STATUS != TW_MR_DATA_ACK)
    {
        std_error_catch_custom(error, (int)TW_STATUS, I2C_DEFAULT_ERROR_TEXT, FILE_NAME, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}

int i2c_master_read_byte_without_acknowledgement (uint8_t * const byte, std_error_t * const error)
{
    // Configure Control register
    TWCR = (1 << TWINT) | (1 << TWEN);

    // Wait for TWINT flag to set
    while ((TWCR & (1 << TWINT)) == 0U);

    // Read byte from data register
    *byte = TWDR;

    // Check errors
    // Data received, NACK returned
    if (TW_STATUS != TW_MR_DATA_NACK)
    {
        std_error_catch_custom(error, (int)TW_STATUS, I2C_DEFAULT_ERROR_TEXT, FILE_NAME, __LINE__);

        return STD_FAILURE;
    }
    return STD_SUCCESS;
}
