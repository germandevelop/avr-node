/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#ifndef INT_0_H
#define INT_0_H

#include <stdbool.h>

typedef void (*int_0_callback_t)();

typedef enum int_0_edge
{
	EDGE_0_LOW_LEVEL = 0, // The low level of INT0 generates an interrupt request
	EDGE_0_CHANGING,	  // Any logical change on INT0 generates an interrupt request
	EDGE_0_FALLING,		  // The falling edge of INT0 generates an interrupt request
	EDGE_0_RISING		  // The rising edge of INT0 generates an interrupt request

} int_0_edge_t;

typedef struct int_0_config
{
	int_0_edge_t edge;
	bool is_pullup_enabled;
	int_0_callback_t int_0_callback;

} int_0_config_t;

void int_0_start (int_0_config_t const * const init_config);
void int_0_stop ();

#endif // INT_0_H
