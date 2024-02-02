/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#ifndef INT_1_H
#define INT_1_H

#include <stdbool.h>

typedef void (*int_1_callback_t)();

typedef enum int_1_edge
{
	EDGE_1_LOW_LEVEL = 0, // The low level of INT1 generates an interrupt request
	EDGE_1_CHANGING,	  // Any logical change on INT1 generates an interrupt request
	EDGE_1_FALLING,		  // The falling edge of INT1 generates an interrupt request
	EDGE_1_RISING		  // The rising edge of INT1 generates an interrupt request

} int_1_edge_t;

typedef struct int_1_config
{
	int_1_edge_t edge;
	bool is_pullup_enabled;
	int_1_callback_t int_1_callback;

} int_1_config_t;

void int_1_start (int_1_config_t const * const init_config);
void int_1_stop ();

#endif // INT_1_H
