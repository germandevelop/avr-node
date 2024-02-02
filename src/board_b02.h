/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef BOARD_B02_H
#define BOARD_B02_H

#include <stdbool.h>

typedef struct board_basic_state board_basic_state_t;
typedef struct board_extra_state board_extra_state_t;

void board_b02_init ();
void board_b02_process (board_basic_state_t const * const basic_state, board_extra_state_t * const extra_state);
void board_b02_is_interrupt (bool * const is_interrupt);

#endif // BOARD_B02_H