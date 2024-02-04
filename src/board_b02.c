/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "board_b02.h"
#include "board.types.h"

#include <assert.h>

#include <avr/io.h>
#include <avr/power.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "mcu/i2c.h"
#include "mcu/int_1.h"
#include "mcu/pcint_d.h"

#include "devices/bmp280_sensor.h"

#include "std_error/std_error.h"
#include "logger.h"


#define TEMPERATURE_SENSOR_CYCLE_COUNT  8U  //  * 7,5 sec = ~ min

#define INTRUSION_WHITE_AND_RED_CYCLE_COUNT 1U  //  * 7,5 sec = ~ min
#define INTRUSION_WHITE_CYCLE_COUNT         2U  // * 7,5 sec = ~ min

#define SILENCE_WHITE_CYCLE_COUNT       1U  // * 7,5 sec = ~ min
#define SILENCE_GREEN_BLUE_CYCLE_COUNT  3U  // * 7,5 sec = ~ min

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define UNUSED(x) (void)(x)


typedef enum light_strip_color
{
    NO_LIGHT = 0,
    WHITE_AND_RED_LIGHT,    // + Projectors + Buzzer
    WHITE_LIGHT,            // + Projectors
    GREEN_LIGHT,
    BLUE_LIGHT,
    RED_LIGHT               // + Buzzer

} light_strip_color_t;


static volatile bool is_int_1_interrupt;
static volatile bool is_pcint_16_interrupt;

static bool is_door_pir_interrupt;
static bool is_veranda_pir_interrupt;

static light_strip_color_t current_light_color;
static bool is_long_range_pir_enabled;


static void board_b02_int_1_ISR ();
static void board_b02_pcint_16_ISR (pcint_d_state_t state);

static void board_b02_init_i2c ();
static void board_b02_deinit_i2c ();

static void board_b02_temperature_sensor_delay_ms (uint32_t delay_ms);

static void board_b02_init_temperature_sensor ();
static void board_b02_init_door_pir ();
static void board_b02_init_veranda_pir ();

static void board_b02_process_light_strip (board_basic_state_t const * const basic_state, board_extra_state_t * const extra_state);
static void board_b02_process_temperature_sensor (board_basic_state_t const * const basic_state, board_extra_state_t * const extra_state);

static void board_b02_set_light_strip_color (light_strip_color_t color);
static void board_b02_power_on_long_range_pir ();
static void board_b02_power_off_long_range_pir ();

void board_b02_init ()
{
    is_int_1_interrupt      = false;
    is_pcint_16_interrupt   = false;

    is_door_pir_interrupt       = false;
    is_veranda_pir_interrupt    = false;

    current_light_color         = NO_LIGHT;
    is_long_range_pir_enabled   = false;

    pcint_d_init();

    board_b02_init_temperature_sensor();
    board_b02_init_door_pir();
    board_b02_init_veranda_pir();

    return;
}


void board_b02_init_temperature_sensor ()
{
    std_error_t error;
    std_error_init(&error);

    // Init BMP280 sensor
    board_b02_init_i2c();

    bmp280_sensor_config_t config;
    config.read_i2c_callback    = i2c_master_read_byte_array;
    config.write_i2c_callback   = i2c_master_write_byte_array;
    config.delay_callback       = board_b02_temperature_sensor_delay_ms;

    if (bmp280_sensor_init(&config, &error) != STD_SUCCESS)
    {
        LOG("%s\r\n", error.text);
    }

    board_b02_deinit_i2c();

    return;
}

void board_b02_init_door_pir ()
{
    // Init INT_1 (DOOR or LONG RANGE pir interrupt)
    int_1_config_t config;
    config.edge                 = EDGE_1_RISING;
    config.is_pullup_enabled    = false;
    config.int_1_callback       = board_b02_int_1_ISR;

    int_1_start(&config);

    return;
}

void board_b02_init_veranda_pir ()
{
    // Init PCINT_16 (VERANDA pir interrupt)
    pcint_d_pin_config_t config;
    config.pin                  = PCINT_D_16;
    config.is_pullup_enabled    = false;
    config.pcint_d_callback     = board_b02_pcint_16_ISR;

    pcint_d_add(&config);

    return;
}


void board_b02_process (board_basic_state_t const * const basic_state, board_extra_state_t * const extra_state)
{
    assert(basic_state != NULL);
    assert(extra_state != NULL);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        is_door_pir_interrupt       = is_int_1_interrupt;
        is_veranda_pir_interrupt    = is_pcint_16_interrupt;

        is_int_1_interrupt      = false;
        is_pcint_16_interrupt   = false;
    }

    if (is_door_pir_interrupt == true)
    {
        LOG("DOOR int\r\n");
    }

    if (is_veranda_pir_interrupt == true)
    {
        LOG("VER int\r\n");
    }

    board_b02_process_light_strip(basic_state, extra_state);
    board_b02_process_temperature_sensor(basic_state, extra_state);

    if ((basic_state->is_dark == true) && (is_long_range_pir_enabled == false))
    {
        board_b02_power_on_long_range_pir();
    }
    else if ((basic_state->is_dark == false) && (is_long_range_pir_enabled == true))
    {
        board_b02_power_off_long_range_pir();
    }

    extra_state->is_light_on = (current_light_color != NO_LIGHT);

    return;
}


void board_b02_process_light_strip (board_basic_state_t const * const basic_state, board_extra_state_t * const extra_state)
{
    assert(basic_state != NULL);
    assert(extra_state != NULL);

    static size_t prev_cycle_count = 0U;

    if (basic_state->global_cycle_count < prev_cycle_count)
    {
        prev_cycle_count = basic_state->global_cycle_count;
    }

    // Alarm mode
    if (basic_state->current_mode == ALARM)
    {
        if (prev_cycle_count != basic_state->global_cycle_count)
        {
            if (basic_state->is_dark == true)
            {
                if (current_light_color != WHITE_AND_RED_LIGHT)
                {
                    board_b02_set_light_strip_color(WHITE_AND_RED_LIGHT);
                    current_light_color = WHITE_AND_RED_LIGHT;
                }
                else
                {
                    board_b02_set_light_strip_color(WHITE_LIGHT);
                    current_light_color = WHITE_LIGHT;
                }
            }
            else
            {
                if (current_light_color != RED_LIGHT)
                {
                    board_b02_set_light_strip_color(RED_LIGHT);
                    current_light_color = RED_LIGHT;
                }
                else
                {
                    board_b02_set_light_strip_color(NO_LIGHT);
                    current_light_color = NO_LIGHT;
                }
            }
            prev_cycle_count = basic_state->global_cycle_count;
        }
    }

    // Intrusion mode
    else if (basic_state->current_mode == INTRUSION)
    {
        if ((current_light_color != WHITE_AND_RED_LIGHT) && (current_light_color != WHITE_LIGHT))
        {
            const bool is_pir_interrupt = (is_door_pir_interrupt == true) || (is_veranda_pir_interrupt == true);
            const bool is_time_to_start = (basic_state->is_dark == true) && ((is_pir_interrupt == true) || (basic_state->is_enable_light_command == true));

            if (is_time_to_start == true)
            {
                board_b02_set_light_strip_color(WHITE_AND_RED_LIGHT);
                current_light_color = WHITE_AND_RED_LIGHT;

                prev_cycle_count = basic_state->global_cycle_count;

                if (is_pir_interrupt == true)
                {
                    size_t i = 0U;

                    extra_state->send_msg_array[LIGHT_MSG].header.source        = NODE_B02;
                    extra_state->send_msg_array[LIGHT_MSG].header.dest_array[i] = NODE_B01;
                    ++i;
                    extra_state->send_msg_array[LIGHT_MSG].header.dest_array[i] = NODE_T01;
                    ++i;
                    extra_state->send_msg_array[LIGHT_MSG].header.dest_array_size = i;

                    extra_state->send_msg_array[LIGHT_MSG].cmd_id   = SET_LIGHT;
                    extra_state->send_msg_array[LIGHT_MSG].value_0  = (int32_t)LIGHT_ON;

                    extra_state->send_msg_retry_count[LIGHT_MSG] = 0U;

                    extra_state->is_msg_to_send = true;
                }
            }
        }
        else if (current_light_color != WHITE_LIGHT)
        {
            const bool is_time_to_stop = (basic_state->global_cycle_count - prev_cycle_count) > INTRUSION_WHITE_AND_RED_CYCLE_COUNT;

            if (is_time_to_stop == true)
            {
                board_b02_set_light_strip_color(WHITE_LIGHT);
                current_light_color = WHITE_LIGHT;

                prev_cycle_count = basic_state->global_cycle_count;
            }
        }
        else
        {
            const bool is_time_to_stop = (basic_state->global_cycle_count - prev_cycle_count) > INTRUSION_WHITE_CYCLE_COUNT;

            if (is_time_to_stop == true)
            {
                board_b02_set_light_strip_color(NO_LIGHT);
                current_light_color = NO_LIGHT;

                prev_cycle_count = basic_state->global_cycle_count;
            }
        }
    }

    // Silence mode
    else if (basic_state->current_mode == SILENCE)
    {
        if (current_light_color != WHITE_LIGHT)
        {
            const bool is_time_to_start = (basic_state->is_dark == true) && ((is_door_pir_interrupt == true) || (basic_state->is_enable_light_command == true));

            if (is_time_to_start == true)
            {
                board_b02_set_light_strip_color(WHITE_LIGHT);
                current_light_color = WHITE_LIGHT;

                prev_cycle_count = basic_state->global_cycle_count;

                if (is_door_pir_interrupt == true)
                {
                    size_t i = 0U;

                    extra_state->send_msg_array[LIGHT_MSG].header.source        = NODE_B02;
                    extra_state->send_msg_array[LIGHT_MSG].header.dest_array[i] = NODE_T01;
                    ++i;
                    extra_state->send_msg_array[LIGHT_MSG].header.dest_array_size = i;

                    extra_state->send_msg_array[LIGHT_MSG].cmd_id   = SET_LIGHT;
                    extra_state->send_msg_array[LIGHT_MSG].value_0  = (int32_t)LIGHT_ON;

                    extra_state->send_msg_retry_count[LIGHT_MSG] = 0U;

                    extra_state->is_msg_to_send = true;
                }
            }
            else
            {
                const bool is_time_to_stop = (basic_state->global_cycle_count - prev_cycle_count) > SILENCE_GREEN_BLUE_CYCLE_COUNT;

                if (is_time_to_stop == true)
                {
                    board_b02_set_light_strip_color(NO_LIGHT);
                    current_light_color = NO_LIGHT;

                    prev_cycle_count = basic_state->global_cycle_count;
                }
                else
                {
                    if (current_light_color == BLUE_LIGHT)
                    {
                        board_b02_set_light_strip_color(GREEN_LIGHT);
                        current_light_color = GREEN_LIGHT;
                    }
                    else if (current_light_color == GREEN_LIGHT)
                    {
                        board_b02_set_light_strip_color(BLUE_LIGHT);
                        current_light_color = BLUE_LIGHT;
                    }
                }
            }
        }
        else
        {
            const bool is_time_to_stop = (basic_state->global_cycle_count - prev_cycle_count) > SILENCE_WHITE_CYCLE_COUNT;

            if (is_time_to_stop == true)
            {
                board_b02_set_light_strip_color(GREEN_LIGHT);
                current_light_color = GREEN_LIGHT;

                prev_cycle_count = basic_state->global_cycle_count;
            }
        }
    }

    if ((basic_state->current_mode != basic_state->new_mode) || (basic_state->is_disable_light_command == true))
    {
        board_b02_set_light_strip_color(NO_LIGHT);
        current_light_color = NO_LIGHT;
    }

    return;
}

void board_b02_process_temperature_sensor (board_basic_state_t const * const basic_state, board_extra_state_t * const extra_state)
{
    assert(basic_state != NULL);
    assert(extra_state != NULL);

    static size_t prev_cycle_count = 0U;

    if (basic_state->global_cycle_count < prev_cycle_count)
    {
        prev_cycle_count = basic_state->global_cycle_count;
    }

    const bool is_measuring_cycle = (basic_state->global_cycle_count - prev_cycle_count) > TEMPERATURE_SENSOR_CYCLE_COUNT;

    if (is_measuring_cycle == true)
    {
        std_error_t error;
        std_error_init(&error);

        board_b02_init_i2c();

        bmp280_sensor_data_t data;

        if (bmp280_sensor_read_data(&data, &error) != STD_SUCCESS)
        {
            LOG("%s\r\n", error.text);
        }
        else
        {
            LOG("Press: %d hPa\r\n", (int)data.pressure_hPa);
            LOG("Temp: %d C\r\n", (int)data.temperature_C);

            size_t i = 0U;

            extra_state->send_msg_array[TEMPERATURE_MSG].header.source          = NODE_B02;
            extra_state->send_msg_array[TEMPERATURE_MSG].header.dest_array[i]   = NODE_B01;
            ++i;
            extra_state->send_msg_array[TEMPERATURE_MSG].header.dest_array_size = i;

            extra_state->send_msg_array[TEMPERATURE_MSG].cmd_id     = UPDATE_TEMPERATURE;
            extra_state->send_msg_array[TEMPERATURE_MSG].value_0    = (int32_t)data.pressure_hPa;
            extra_state->send_msg_array[TEMPERATURE_MSG].value_1    = data.temperature_C;

            extra_state->send_msg_retry_count[TEMPERATURE_MSG] = 0U;

            extra_state->is_msg_to_send = true;
        }

        board_b02_deinit_i2c();

        prev_cycle_count = basic_state->global_cycle_count;
    }
    return;
}

void board_b02_set_light_strip_color (light_strip_color_t color)
{
    if (color == WHITE_AND_RED_LIGHT)
    {
        LOG("enable WHITE + RED + PRJCTS + BUZZ\r\n");
    }
    else if (color == WHITE_LIGHT)
    {
        LOG("enable WHITE + PRJCTS\r\n");
    }
    else if (color == GREEN_LIGHT)
    {
        LOG("enable GREEN\r\n");
    }
    else if (color == BLUE_LIGHT)
    {
        LOG("enable BLUE\r\n");
    }
    else if (color == RED_LIGHT)
    {
        LOG("enable RED + BUZZER\r\n");
    }
    else
    {
        LOG("disable ALL\r\n");
    }
    return;
}

void board_b02_power_on_long_range_pir ()
{
    LOG("power on LONG RANGE pir\r\n");
    is_long_range_pir_enabled = true;

    return;
}

void board_b02_power_off_long_range_pir ()
{
    LOG("power off LONG RANGE pir\r\n");
    is_long_range_pir_enabled = false;

    return;
}


void board_b02_is_interrupt (bool * const is_interrupt)
{
    *is_interrupt = (is_int_1_interrupt != false) || (is_pcint_16_interrupt != false);

    return;
}


void board_b02_temperature_sensor_delay_ms (uint32_t delay_ms)
{
    UNUSED(delay_ms);

    _delay_ms(160);

    return;
}


void board_b02_int_1_ISR ()
{
    is_int_1_interrupt = true;

    return;
}

void board_b02_pcint_16_ISR (pcint_d_state_t state)
{
    if (state == STATE_D_HIGH)
    {
        is_pcint_16_interrupt = true;
    }
    return;
}


void board_b02_init_i2c ()
{
    power_twi_enable();

    i2c_config_t config;
    config.frequency            = SCL_400KHz;
    config.is_pullup_enabled    = false;

    i2c_master_init(&config);

    return;
}

void board_b02_deinit_i2c ()
{
    i2c_master_deinit();

    power_twi_disable();

    return;
}
