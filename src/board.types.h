/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef BOARD_TYPES_H
#define BOARD_TYPES_H

#include <stdbool.h>

#include "node/node.types.h"

typedef struct board_basic_state
{
    size_t global_cycle_count;

    bool is_dark;
    node_mode_id_t current_mode;
    node_mode_id_t new_mode;

    bool is_enable_light_command;
    bool is_disable_light_command;

} board_basic_state_t;

typedef enum board_msg_id
{
    LIGHT_MSG = 0,
    TEMPERATURE_MSG,
    BOARD_MSG_SIZE

} board_msg_id_t;

typedef struct board_extra_state
{
    bool is_light_on;
    node_msg_t send_msg_array[BOARD_MSG_SIZE];
    size_t send_msg_retry_count[BOARD_MSG_SIZE];
    bool is_msg_to_send;

} board_extra_state_t;

#endif // BOARD_TYPES_H