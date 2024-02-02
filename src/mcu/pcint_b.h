/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#ifndef PCINT_B_H
#define PCINT_B_H

#include <stdbool.h>

typedef enum pcint_b_state
{
    STATE_B_LOW = 0,
    STATE_B_HIGH,
    STATE_B_UNKNOWN

} pcint_b_state_t;

typedef void (*pcint_b_callback_t)(pcint_b_state_t state);

typedef enum pcint_b_pin
{
    PCINT_B_0 = 0,
    PCINT_B_COUNT

} pcint_b_pin_t;

typedef struct pcint_b_pin_config
{
    pcint_b_pin_t pin;
    bool is_pullup_enabled;
    pcint_b_callback_t pcint_b_callback;

} pcint_b_pin_config_t;

void pcint_b_init ();

void pcint_b_add (pcint_b_pin_config_t const * const config);
void pcint_b_remove (pcint_b_pin_t pin);
void pcint_b_remove_all ();

#endif // PCINT_B_H
