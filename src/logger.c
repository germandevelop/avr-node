/************************************************************
 *   Author : German Mundinger
 *   Date   : 2020
 ************************************************************/

#include "logger.h"

#include <assert.h>

#define UNUSED(x) (void)(x)


static logger_config_t config;

#ifndef NDEBUG
static FILE logger_out;
#endif // NDEBUG


#ifndef NDEBUG
static int logger_send_char (char symbol, FILE *stream);
#endif // NDEBUG

void logger_init (logger_config_t const * const init_config)
{
	assert(init_config != NULL);
	assert(init_config->write_byte_callback != NULL);

	config = *init_config;

#ifndef NDEBUG
	fdev_setup_stream(&logger_out, logger_send_char, NULL, _FDEV_SETUP_WRITE);
	stdout = &logger_out;
	stderr = &logger_out;
#endif // NDEBUG

	return;
}

#ifndef NDEBUG

int logger_send_char (char symbol, FILE *stream)
{
	UNUSED(stream);

	config.write_byte_callback((uint8_t)(symbol));

	return 0;
}

#endif // NDEBUG
