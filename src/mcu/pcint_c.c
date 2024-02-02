/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#include "pcint_c.h"

#include <stddef.h>
#include <assert.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "mcu/config.h"


#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static pcint_c_pin_config_t config_array[PCINT_C_COUNT];
static pcint_c_state_t previous_state_array[ARRAY_SIZE(config_array)];

static void pcint_c_reset ();

void pcint_c_init ()
{
    pcint_c_reset();

    return;
}

void pcint_c_add (pcint_c_pin_config_t const * const config)
{
    assert(config != NULL);
    assert(config->pcint_c_callback != NULL);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        config_array[config->pin]           = *config;
        previous_state_array[config->pin]   = STATE_C_UNKNOWN;
    }

    if (config->pin == PCINT_C_11)
    {
        // Set PCINT11 pin as input
        DDR_PCINT_11 &= ~(1 << DDR_N_PCINT_11);

        // Enable / Disable the internal pull-up resistor for PCINT11
        if (config_array[config->pin].is_pullup_enabled == true)
        {
            PORT_PCINT_11 |= (1 << PORT_N_PCINT_11);
        }
        else
        {
            PORT_PCINT_11 &= ~(1 << PORT_N_PCINT_11);
        }

        // Enable pin interrupt mask
        PCMSK1 |= (1 << PCINT11);
    }

    // Enable interrupt for port
    PCICR |= (1 << PCIE1);

    return;
}

void pcint_c_remove (pcint_c_pin_t pin)
{
    if (pin == PCINT_C_11)
    {
        // Disable pin interrupt mask
        PCMSK1 &= ~(1 << PCINT11);
    }

    bool are_active_interruptions = false;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        config_array[pin].pcint_c_callback  = NULL;
        previous_state_array[pin]           = STATE_C_UNKNOWN;

        for (size_t i = 0U; i < ARRAY_SIZE(config_array); ++i)
        {
            if (config_array[i].pcint_c_callback != NULL)
            {
                are_active_interruptions = true;

                break;
            }
        }
    }

    // Try to disable interrupt for port
    if (are_active_interruptions == false)
    {
        PCICR &= ~(1 << PCIE1);
    }
    return;
}

void pcint_c_remove_all ()
{
    // Disable interrupt for port
    PCICR &= ~(1 << PCIE1);

    // Disable all pins interrupt mask
    PCMSK1 &= ~((1 << PCINT8) | (1 << PCINT9) | (1 << PCINT10) | (1 << PCINT11));
    PCMSK1 &= ~((1 << PCINT12) | (1 << PCINT13) | (1 << PCINT14));

    pcint_c_reset();

    return;
}

void pcint_c_reset ()
{
    for (size_t i = 0U; i < ARRAY_SIZE(config_array); ++i)
    {
        config_array[i].pcint_c_callback    = NULL;
        previous_state_array[i]             = STATE_C_UNKNOWN;
    }
    return;
}

ISR (PCINT1_vect)
{
    for (size_t i = 0U; i < ARRAY_SIZE(config_array); ++i)
    {
        if (config_array[i].pcint_c_callback != NULL)
        {
            if (config_array[i].pin == PCINT_C_11)
            {
                pcint_c_state_t current_state = STATE_C_LOW;

                if ((PIN_PCINT_11 & (1 << PIN_N_PCINT_11)) == (1 << PIN_N_PCINT_11))
                {
                    current_state = STATE_C_HIGH;
                }

                if (previous_state_array[i] != current_state)
                {
                    previous_state_array[i] = current_state;

                    config_array[i].pcint_c_callback(current_state);
                }
            }
        }
    }
}
