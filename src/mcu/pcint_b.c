/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#include "pcint_b.h"

#include <stddef.h>
#include <assert.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "mcu/config.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static pcint_b_pin_config_t config_array[PCINT_B_COUNT];
static pcint_b_state_t previous_state_array[ARRAY_SIZE(config_array)];


static void pcint_b_reset ();

void pcint_b_init ()
{
    pcint_b_reset();

    return;
}

void pcint_b_add (pcint_b_pin_config_t const * const config)
{
    assert(config != NULL);
    assert(config->pcint_b_callback != NULL);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        config_array[config->pin]           = *config;
        previous_state_array[config->pin]   = STATE_B_UNKNOWN;
    }

    if (config->pin == PCINT_B_0)
    {
        // Set PCINT0 pin as input
        DDR_PCINT_0 &= ~(1 << DDR_N_PCINT_0);

        // Enable / Disable the internal pull-up resistor for PCINT0
        if (config_array[config->pin].is_pullup_enabled == true)
        {
            PORT_PCINT_0 |= (1 << PORT_N_PCINT_0);
        }
        else
        {
            PORT_PCINT_0 &= ~(1 << PORT_N_PCINT_0);
        }

        // Enable pin interrupt mask
        PCMSK0 |= (1 << PCINT0);
    }

    // Enable interrupt for port
    PCICR |= (1 << PCIE0);

    return;
}

void pcint_b_remove (pcint_b_pin_t pin)
{
    if (pin == PCINT_B_0)
    {
        // Disable pin interrupt mask
        PCMSK0 &= ~(1 << PCINT0);
    }

    bool are_active_interruptions = false;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        config_array[pin].pcint_b_callback  = NULL;
        previous_state_array[pin]           = STATE_B_UNKNOWN;

        for (size_t i = 0U; i < ARRAY_SIZE(config_array); ++i)
        {
            if (config_array[i].pcint_b_callback != NULL)
            {
                are_active_interruptions = true;

                break;
            }
        }
    }

    // Try to disable interrupt for port
    if (are_active_interruptions == false)
    {
        PCICR &= ~(1 << PCIE0);
    }
    return;
}

void pcint_b_remove_all ()
{
    // Disable interrupt for port
    PCICR &= ~(1 << PCIE0);

    // Disable all pins interrupt mask
    PCMSK0 &= ~((1 << PCINT0) | (1 << PCINT1) | (1 << PCINT2) | (1 << PCINT3));
    PCMSK0 &= ~((1 << PCINT4) | (1 << PCINT5) | (1 << PCINT6) | (1 << PCINT7));

    pcint_b_reset();

    return;
}

void pcint_b_reset ()
{
    for (size_t i = 0U; i < ARRAY_SIZE(config_array); ++i)
    {
        config_array[i].pcint_b_callback    = NULL;
        previous_state_array[i]             = STATE_B_UNKNOWN;
    }
    return;
}

ISR (PCINT0_vect)
{
    for (size_t i = 0U; i < ARRAY_SIZE(config_array); ++i)
    {
        if (config_array[i].pcint_b_callback != NULL)
        {
            if (config_array[i].pin == PCINT_B_0)
            {
                pcint_b_state_t current_state = STATE_B_LOW;

                if ((PIN_PCINT_0 & (1 << PIN_N_PCINT_0)) == (1 << PIN_N_PCINT_0))
                {
                    current_state = STATE_B_HIGH;
                }

                if (previous_state_array[i] != current_state)
                {
                    previous_state_array[i] = current_state;
                    
                    config_array[i].pcint_b_callback(current_state);
                }
            }
        }
    }
}
