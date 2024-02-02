/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#ifndef PCINT_D_H
#define PCINT_D_H

#include <stdbool.h>

typedef enum pcint_d_state
{
    STATE_D_LOW = 0,
    STATE_D_HIGH,
    STATE_D_UNKNOWN

} pcint_d_state_t;

typedef void (*pcint_d_callback_t)(pcint_d_state_t state);

typedef enum pcint_d_pin
{
    PCINT_D_16 = 0,
    PCINT_D_21,
    PCINT_D_COUNT

} pcint_d_pin_t;

typedef struct pcint_d_pin_config
{
    pcint_d_pin_t pin;
    bool is_pullup_enabled;
    pcint_d_callback_t pcint_d_callback;

} pcint_d_pin_config_t;

void pcint_d_init ();

void pcint_d_add (pcint_d_pin_config_t const * const config);
void pcint_d_remove (pcint_d_pin_t pin);
void pcint_d_remove_all ();

#endif // PCINT_D_H
