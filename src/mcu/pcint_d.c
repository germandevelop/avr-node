/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#include "pcint_d.h"

#include <stddef.h>
#include <assert.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "mcu/config.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static pcint_d_pin_config_t config_array[PCINT_D_COUNT];
static pcint_d_state_t previous_state_array[ARRAY_SIZE(config_array)];


static void pcint_d_reset ();

void pcint_d_init ()
{
    pcint_d_reset();

    return;
}

void pcint_d_add (pcint_d_pin_config_t const * const config)
{
    assert(config != NULL);
    assert(config->pcint_d_callback != NULL);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        config_array[config->pin]           = *config;
        previous_state_array[config->pin]   = STATE_D_UNKNOWN;
    }

    if (config->pin == PCINT_D_16)
    {
        // Set PCINT16 pin as input
        DDR_PCINT_D &= ~(1 << PORT_N_PCINT_16);

        // Enable / Disable the internal pull-up resistor for PCINT21
        if (config_array[config->pin].is_pullup_enabled == true)
        {
            PORT_PCINT_D |= (1 << PORT_N_PCINT_16);
        }
        else
        {
            PORT_PCINT_D &= ~(1 << PORT_N_PCINT_16);
        }

        // Enable pin interrupt mask
        PCMSK2 |= (1 << PCINT16);
    }
    else if (config->pin == PCINT_D_21)
    {
        // Set PCINT21 pin as input
        DDR_PCINT_D &= ~(1 << PORT_N_PCINT_21);

        // Enable / Disable the internal pull-up resistor for PCINT21
        if (config_array[config->pin].is_pullup_enabled == true)
        {
            PORT_PCINT_D |= (1 << PORT_N_PCINT_21);
        }
        else
        {
            PORT_PCINT_D &= ~(1 << PORT_N_PCINT_21);
        }

        // Enable pin interrupt mask
        PCMSK2 |= (1 << PCINT21);
    }

    // Enable interrupt for port
    PCICR |= (1 << PCIE2);

    return;
}

void pcint_d_remove (pcint_d_pin_t pin)
{
    if (pin == PCINT_D_16)
    {
        // Disable pin interrupt mask
        PCMSK2 &= ~(1 << PCINT16);
    }
    else if (pin == PCINT_D_21)
    {
        // Disable pin interrupt mask
        PCMSK2 &= ~(1 << PCINT21);
    }

    bool are_active_interruptions = false;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        config_array[pin].pcint_d_callback  = NULL;
        previous_state_array[pin]           = STATE_D_UNKNOWN;

        for (size_t i = 0U; i < ARRAY_SIZE(config_array); ++i)
        {
            if (config_array[i].pcint_d_callback != NULL)
            {
                are_active_interruptions = true;

                break;
            }
        }
    }

    // Try to disable interrupt for port
    if (are_active_interruptions == false)
    {
        PCICR &= ~(1 << PCIE2);
    }
    return;
}

void pcint_d_remove_all ()
{
    // Disable interrupt for port
    PCICR &= ~(1 << PCIE2);

    // Disable all pins interrupt mask
    PCMSK2 &= ~((1 << PCINT16) | (1 << PCINT17) | (1 << PCINT18) | (1 << PCINT19));
    PCMSK2 &= ~((1 << PCINT20) | (1 << PCINT21) | (1 << PCINT22) | (1 << PCINT23));

    pcint_d_reset();

    return;
}

void pcint_d_reset ()
{
    for (size_t i = 0U; i < ARRAY_SIZE(config_array); ++i)
    {
        config_array[i].pcint_d_callback    = NULL;
        previous_state_array[i]             = STATE_D_UNKNOWN;
    }
    return;
}

ISR (PCINT2_vect)
{
    for (size_t i = 0U; i < ARRAY_SIZE(config_array); ++i)
    {
        if (config_array[i].pcint_d_callback != NULL)
        {
            if (config_array[i].pin == PCINT_D_16)
            {
                pcint_d_state_t current_state = STATE_D_LOW;

                if ((PIN_PCINT_D & (1 << PIN_N_PCINT_16)) == (1 << PIN_N_PCINT_16))
                {
                    current_state = STATE_D_HIGH;
                }

                if (previous_state_array[i] != current_state)
                {
                    previous_state_array[i] = current_state;

                    config_array[i].pcint_d_callback(current_state);
                }
            }
            else if (config_array[i].pin == PCINT_D_21)
            {
                pcint_d_state_t current_state = STATE_D_LOW;

                if ((PIN_PCINT_D & (1 << PIN_N_PCINT_21)) == (1 << PIN_N_PCINT_21))
                {
                    current_state = STATE_D_HIGH;
                }

                if (previous_state_array[i] != current_state)
                {
                    previous_state_array[i] = current_state;

                    config_array[i].pcint_d_callback(current_state);
                }
            }
        }
    }
}
