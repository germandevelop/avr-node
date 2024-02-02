/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#ifndef PCINT_C_H
#define PCINT_C_H

#include <stdbool.h>

typedef enum pcint_c_state
{
    STATE_C_LOW = 0,
    STATE_C_HIGH,
    STATE_C_UNKNOWN

} pcint_c_state_t;

typedef void (*pcint_c_callback_t)(pcint_c_state_t state);

typedef enum pcint_c_pin
{
    PCINT_C_11 = 0,
    PCINT_C_COUNT

} pcint_c_pin_t;

typedef struct pcint_c_pin_config
{
    pcint_c_pin_t pin;
    bool is_pullup_enabled;
    pcint_c_callback_t pcint_c_callback;

} pcint_c_pin_config_t;

void pcint_c_init ();

void pcint_c_add (pcint_c_pin_config_t const * const config);
void pcint_c_remove (pcint_c_pin_t pin);
void pcint_c_remove_all ();

#endif // PCINT_C_H
