/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#include "int_1.h"

#include <assert.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "mcu/config.h"


static int_1_config_t config;


void int_1_start (int_1_config_t const * const init_config)
{
    assert(init_config != NULL);
    assert(init_config->int_1_callback != NULL);

    config = *init_config;

    // Set INT1 pin as input
    DDR_INT1 &= ~(1 << DDR_N_INT1);

    // Enable / Disable the internal pull-up resistor for INT1
    if (config.is_pullup_enabled == true)
    {
        PORT_INT1 |= (1 << PORT_N_INT1);
    }
    else
    {
        PORT_INT1 &= ~(1 << PORT_N_INT1);
    }

    // Configure external interrupt INT1
    if (config.edge == EDGE_1_LOW_LEVEL)
    {
        EICRA &= ~(1 << ISC10);
        EICRA &= ~(1 << ISC11);
    }
    else if (config.edge == EDGE_1_CHANGING)
    {
        EICRA |= (1 << ISC10);
        EICRA &= ~(1 << ISC11);
    }
    else if (config.edge == EDGE_1_FALLING)
    {
        EICRA &= ~(1 << ISC10);
        EICRA |= (1 << ISC11);
    }
    else if (config.edge == EDGE_1_RISING)
    {
        EICRA |= (1 << ISC10);
        EICRA |= (1 << ISC11);
    }

    // Enable external interrupt INT0
    EIMSK |= (1 << INT1);

    return;
}

void int_1_stop ()
{
    // Disable external interrupt INT0
    EIMSK &= ~(1 << INT1);

    // Restore INT0 to default configuration
    EICRA &= ~(1 << ISC10);
    EICRA &= ~(1 << ISC11);

    return;
}

ISR (INT1_vect)
{
    config.int_1_callback();
}
