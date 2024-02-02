/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#include "int_0.h"

#include <assert.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "mcu/config.h"


static int_0_config_t config;


void int_0_start (int_0_config_t const * const init_config)
{
    assert(init_config != NULL);
    assert(init_config->int_0_callback != NULL);

    config = *init_config;

    // Set INT0 pin as input
    DDR_INT0 &= ~(1 << DDR_N_INT0);

    // Enable / Disable the internal pull-up resistor for INT0
    if (config.is_pullup_enabled == true)
    {
        PORT_INT0 |= (1 << PORT_N_INT0);
    }
    else
    {
        PORT_INT0 &= ~(1 << PORT_N_INT0);
    }

    // Configure external interrupt INT0
    if (config.edge == EDGE_0_LOW_LEVEL)
    {
        EICRA &= ~(1 << ISC00);
        EICRA &= ~(1 << ISC01);
    }
    else if (config.edge == EDGE_0_CHANGING)
    {
        EICRA |= (1 << ISC00);
        EICRA &= ~(1 << ISC01);
    }
    else if (config.edge == EDGE_0_FALLING)
    {
        EICRA &= ~(1 << ISC00);
        EICRA |= (1 << ISC01);
    }
    else if (config.edge == EDGE_0_RISING)
    {
        EICRA |= (1 << ISC00);
        EICRA |= (1 << ISC01);
    }

    // Enable external interrupt INT0
    EIMSK |= (1 << INT0);

    return;
}

void int_0_stop ()
{
    // Disable external interrupt INT0
    EIMSK &= ~(1 << INT0);

    // Restore INT0 to default configuration
    EICRA &= ~(1 << ISC00);
    EICRA &= ~(1 << ISC01);

    return;
}

ISR (INT0_vect)
{
    config.int_0_callback();
}
