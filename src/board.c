/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "board.h"
#include "board.types.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "mcu/uart.h"
#include "mcu/spi.h"
#include "mcu/int_0.h"
#include "mcu/timer_1.h"
#include "mcu/adc.h"

#include "tcp_client.h"
#include "node.mapper.h"

#include "board_b02.h"

#include "std_error/std_error.h"
#include "logger.h"


#define LIGHT_SENSOR_CYCLE_COUNT    4U      //  * 7,5 sec = ~ min
#define LIGHT_SENSOR_DARK_THRESHOLD 100U 

#define MESSAGE_SEND_RETRY_COUNT 4U

#define UART_BAUDRATE 9600U

#define W5500_DDR_CS    DDRD
#define W5500_PORT_CS   PORTD
#define W5500_PIN_CS    PORTD4

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define UNUSED(x) (void)(x)


typedef void (*board_extra_init_callback_t) ();
typedef void (*board_extra_process_callback_t) (board_basic_state_t const * const basic_state, board_extra_state_t * const extra_state);
typedef void (*board_extra_is_interrupt_callback_t) (bool * const is_interrupt);

typedef struct board_extra_strategy
{
    board_extra_init_callback_t init_callback;
    board_extra_process_callback_t process_callback;
    board_extra_is_interrupt_callback_t is_interrupt_callback;

} board_extra_strategy_t;

typedef enum timer_1_gpio
{
    GPIO_A = 0,
    GPIO_B

} timer_1_gpio_t;


static volatile size_t timer1_overflow_count;
static volatile bool is_int_0_interrupt;

static bool is_w5500_interrupt;

static board_basic_state_t basic_state;
static board_extra_state_t extra_state;

static tcp_msg_t tcp_msg;

static node_id_t node_id;
static board_extra_strategy_t extra_strategy;


static void board_timer1_overflow_ISR ();
static void board_int_0_ISR ();

static void w5500_spi_select ();
static void w5500_spi_unselect ();

static void board_init_timer1 (timer_1_gpio_t gpio);
static void board_deinit_timer1 ();
static void board_init_spi ();
static void board_deinit_spi ();
static void board_init_adc ();
static void board_deinit_adc ();

static void board_init_logging ();
static void board_init_strategy ();
static void board_init_tcp_client ();

static void board_process_tcp_client ();
static void board_process_light_sensor ();
static void board_process_led ();

void board_init ()
{
    wdt_enable(WDTO_8S);
    
    power_all_disable();
    set_sleep_mode(SLEEP_MODE_IDLE);

    timer1_overflow_count   = 0U;
    is_int_0_interrupt      = false;

    is_w5500_interrupt = false;

    basic_state.global_cycle_count          = 0U;
    basic_state.is_dark                     = false;
    basic_state.is_enable_light_command     = false;
    basic_state.is_disable_light_command    = false;

    for (size_t i = 0U; i < ARRAY_SIZE(extra_state.send_msg_array); ++i)
    {
        extra_state.send_msg_retry_count[i] = MESSAGE_SEND_RETRY_COUNT;
    }
    extra_state.is_msg_to_send  = false;
    extra_state.is_light_on     = false;

    tcp_msg.size = 0U;

#ifndef NDEBUG
    board_init_logging();
#endif // NDEBUG

    board_init_strategy();
    board_init_tcp_client();

    extra_strategy.init_callback();

    board_init_timer1(GPIO_A);

    //_delay_ms(1000);

    sei();

    return;
}

void board_launch ()
{
    while (true)
    {
        wdt_reset();

        LOG("Loop\r\n");

        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            basic_state.global_cycle_count = timer1_overflow_count;

            is_w5500_interrupt = is_int_0_interrupt;
            is_int_0_interrupt = false;
        }

        board_process_tcp_client();
        extra_strategy.process_callback(&basic_state, &extra_state);
        board_process_light_sensor();
        board_process_led();

        basic_state.current_mode = basic_state.new_mode;
        basic_state.is_enable_light_command = false;
        basic_state.is_disable_light_command = false;

        wdt_reset();

        // Enter to sleep mode
        if (extra_state.is_msg_to_send != true)
        {
            cli();

            bool is_extra_interrupt;
            extra_strategy.is_interrupt_callback(&is_extra_interrupt);

            const bool is_interrupt = (is_int_0_interrupt != false) || (is_extra_interrupt != false);

            if (is_interrupt != true)
            {
                sleep_enable();
                sleep_bod_disable();
                sei();
                sleep_cpu();
                sleep_disable();
            }
            sei();
        }
    }
}

void board_init_logging ()
{
    // Init UART
    power_usart0_enable();

    uart_config_t uart_config;
    uart_config.baud_rate = UART_BAUDRATE;

    uart_init(&uart_config);

    // Init logger
    logger_config_t logger_config;
    logger_config.write_byte_callback = uart_write_byte;

    logger_init(&logger_config);

    LOG("\r\n__LOGGER__\r\n");

    return;
}

void board_init_strategy ()
{
    node_id = NODE_B02;

    extra_strategy.init_callback            = board_b02_init;
    extra_strategy.process_callback         = board_b02_process;
    extra_strategy.is_interrupt_callback    = board_b02_is_interrupt;

    return;
}

void board_init_tcp_client ()
{
    std_error_t error;
    std_error_init(&error);

    // Init CS pin
    W5500_DDR_CS |= (1 << W5500_PIN_CS);
    W5500_PORT_CS |= (1 << W5500_PIN_CS);

    // Init SPI
    board_init_spi();

    // Init W5500
    tcp_client_config_t config = { 0 };

    config.spi_select_callback   = w5500_spi_select;
    config.spi_unselect_callback = w5500_spi_unselect;
    config.spi_read_callback     = spi_master_read_byte;
    config.spi_write_callback    = spi_master_write_byte;

    config.mac_address[0] = 0xEA;
    config.mac_address[1] = 0x11;
    config.mac_address[2] = 0x22;
    config.mac_address[3] = 0x33;
    config.mac_address[4] = 0x44;
    config.mac_address[5] = 0xEA;

    config.ip_address[0] = node_ip_address[node_id][0];
    config.ip_address[1] = node_ip_address[node_id][1];
    config.ip_address[2] = node_ip_address[node_id][2];
    config.ip_address[3] = node_ip_address[node_id][3];

    config.netmask[0] = host_netmask[0];
    config.netmask[1] = host_netmask[1];
    config.netmask[2] = host_netmask[2];
    config.netmask[3] = host_netmask[3];

    config.server_ip[0] = host_ip_address[0];
    config.server_ip[1] = host_ip_address[1];
    config.server_ip[2] = host_ip_address[2];
    config.server_ip[3] = host_ip_address[3];

    config.server_port = host_port;

    if (tcp_client_init(&config, &error) != STD_SUCCESS)
    {
        LOG("%s\r\n", error.text);
    }

    // Init INT_0 (W5500 interrupt)
    int_0_config_t int_config;
    int_config.edge                 = EDGE_0_FALLING;
    int_config.is_pullup_enabled    = false;
    int_config.int_0_callback       = board_int_0_ISR;

    int_0_start(&int_config);

    return;
}


void board_process_tcp_client ()
{
    std_error_t error;
    std_error_init(&error);

    // Check interruptions
    if (is_w5500_interrupt == true)
    {
        tcp_client_check_interrupts();
    }

    // Try to receive a message
    tcp_client_receive_message(&tcp_msg);

    if (tcp_msg.size != 0U)
    {
        LOG("In msg: %s\r\n", tcp_msg.buffer);

        node_msg_t node_msg;

        if (node_mapper_deserialize_message(tcp_msg.buffer, &node_msg, &error) != STD_SUCCESS)
        {
            LOG("%s\r\n", error.text);
        }
        else
        {
            bool is_msg_for_this_node = false;

            for (size_t i = 0U; i < node_msg.header.dest_array_size; ++i)
            {
                if (node_id == node_msg.header.dest_array[i])
                {
                    is_msg_for_this_node = true;

                    break;
                }
            }

            if (is_msg_for_this_node == true)
            {
                if (node_msg.cmd_id == SET_LIGHT)
                {
                    if (node_msg.value_0 == (int32_t)(LIGHT_ON))
                    {
                        basic_state.is_enable_light_command = true;
                    }
                    else if (node_msg.value_0 == (int32_t)(LIGHT_OFF))
                    {
                        basic_state.is_disable_light_command = true;
                    }
                }
                else if (node_msg.cmd_id == SET_MODE)
                {
                    basic_state.new_mode = (node_mode_id_t)(node_msg.value_0);
                }
            }
        }
    }

    // Try to connect or reconnect to a server
    if (tcp_client_connect(&error) != STD_SUCCESS)
    {
        LOG("%s\r\n", error.text);
    }

    // Try to send messages
    for (size_t i = 0U; i < ARRAY_SIZE(extra_state.send_msg_array); ++i)
    {
        if (extra_state.send_msg_retry_count[i] < MESSAGE_SEND_RETRY_COUNT)
        {
            node_mapper_serialize_message(&extra_state.send_msg_array[i], tcp_msg.buffer, &tcp_msg.size);

            LOG("Out msg: %s\r\n", tcp_msg.buffer);

            if (tcp_client_send_message(&tcp_msg, &error) != STD_SUCCESS)
            {
                ++extra_state.send_msg_retry_count[i];

                LOG("%s\r\n", error.text);
            }
            else
            {
                extra_state.send_msg_retry_count[i] = MESSAGE_SEND_RETRY_COUNT;
            }
        }
    }
    extra_state.is_msg_to_send = false;

    return;
}

void board_process_light_sensor ()
{
    static size_t prev_cycle_count = 0U;

    if (basic_state.global_cycle_count < prev_cycle_count)
    {
        prev_cycle_count = basic_state.global_cycle_count;
    }

    const bool is_measuring_cycle = (basic_state.global_cycle_count - prev_cycle_count) > LIGHT_SENSOR_CYCLE_COUNT;

    if ((is_measuring_cycle == true) && (extra_state.is_light_on != true))
    {
        board_deinit_timer1();
        _delay_ms(500);
        board_init_adc();

        uint16_t adc_value;
        adc_read_single_shot(&adc_value);

        board_deinit_adc();

        if (basic_state.current_mode == SILENCE)
        {
            board_init_timer1(GPIO_A);
        }
        else
        {
            board_init_timer1(GPIO_B);
        }

        LOG("Light adc: %u\r\n", adc_value);
        
        //uint16_t adc_voltage = (uint16_t)((5.0F * ((float)adc_value / 1023.0F )) * 1000.0F);
        //uint16_t resistance = (uint16_t)((10.0F * ((float)adc_value / 1023.0F )) * 1000.0F);
        //LOG("ADC voltage %u mV\r\n", adc_voltage);
        //LOG("Resistance %u Ohm\r\n", resistance);

        if (adc_value > LIGHT_SENSOR_DARK_THRESHOLD)
        {
            basic_state.is_dark = true;
        }
        else
        {
            basic_state.is_dark = false;
        }

        prev_cycle_count = basic_state.global_cycle_count;
    }
    return;
}

void board_process_led ()
{
    if (basic_state.current_mode != basic_state.new_mode)
    {
        if (basic_state.new_mode == INTRUSION)
        {
            board_deinit_timer1();
            board_init_timer1(GPIO_B);
        }
        else
        {
            board_deinit_timer1();
            board_init_timer1(GPIO_A);
        }
    }
    return;
}


void board_timer1_overflow_ISR ()
{
    ++timer1_overflow_count; // 1 cycle = ~8 seconds

    return;
}

void board_int_0_ISR ()
{
    is_int_0_interrupt = true;

    return;
}


void w5500_spi_select ()
{
    W5500_PORT_CS &= ~(1 << W5500_PIN_CS);

    return;
}

void w5500_spi_unselect ()
{
    W5500_PORT_CS |= (1 << W5500_PIN_CS);

    return;
}


void board_init_timer1 (timer_1_gpio_t gpio)
{
    power_timer1_enable();

    timer_1_config_t timer_1_config;
    timer_1_config.mode                 = TIMER_1_PWM_MODE;
    timer_1_config.prescaler            = TIMER_1_PRESCALER_1024;
    timer_1_config.overflow_callback    = board_timer1_overflow_ISR;

    timer_1_config.period_a = 3000U;
    timer_1_config.period_b = 3000U;
    timer_1_config.compare_a_callback = NULL;
    timer_1_config.compare_b_callback = NULL;

    timer_1_config.top = 50782U; // ~7.5 sec (65535 - max ~8 sec)
    timer_1_config.is_gpio_a_enabled = false;
    timer_1_config.is_gpio_b_enabled = false;

    if (gpio == GPIO_A)
    {
        timer_1_config.is_gpio_a_enabled = true;
    }
    else if (gpio == GPIO_B)
    {
        timer_1_config.is_gpio_b_enabled = true;
    }

    timer_1_start(&timer_1_config);

    return;
}

void board_deinit_timer1 ()
{
    timer_1_stop();

    power_timer1_disable();

    return;
}

void board_init_spi ()
{
    power_spi_enable();

    spi_config_t config;
    config.clock_rate = CPU_FREQUENCY_DIVIDED_BY_2;

    spi_master_init(&config);

    return;
}

void board_deinit_spi ()
{
    spi_master_deinit();

    power_spi_disable();

    return;
}

void board_init_adc ()
{
    power_adc_enable();

    adc_config_t adc_config;
    adc_config.input        = ADC_INPUT_0;
    adc_config.ref_voltage  = ADC_REF_VOLTAGE_AVCC;

    adc_init(&adc_config);

    return;
}

void board_deinit_adc ()
{
    adc_deinit();

    power_adc_disable();

    return;
}
