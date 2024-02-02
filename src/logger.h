/************************************************************
 *   Author : German Mundinger
 *   Date   : 2020
 ************************************************************/

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

typedef void (*write_byte_callback_t)(uint8_t byte);

typedef struct logger_config
{
	write_byte_callback_t write_byte_callback;

} logger_config_t;

void logger_init (logger_config_t const * const init_config);

#ifdef NDEBUG
#define LOG(...) ((void)0U)
#else
#include <stdio.h>
#define LOG(...) printf(__VA_ARGS__)
#endif // NDEBUG

#endif // LOGGER_H
