/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct std_error std_error_t;

typedef struct tcp_msg
{
    char buffer[128];
    size_t size;

} tcp_msg_t;

typedef void (*tcp_client_spi_select_callback_t) ();
typedef void (*tcp_client_spi_rx_callback_t) (uint8_t * const byte);
typedef void (*tcp_client_spi_tx_callback_t) (uint8_t byte);

typedef struct tcp_client_config
{
    tcp_client_spi_select_callback_t spi_select_callback;
    tcp_client_spi_select_callback_t spi_unselect_callback;
    tcp_client_spi_rx_callback_t spi_read_callback;
    tcp_client_spi_tx_callback_t spi_write_callback;

    uint8_t mac_address[6];
    uint8_t ip_address[4];
    uint8_t netmask[4];

    uint8_t server_ip[4];
    uint16_t server_port;

} tcp_client_config_t;

int tcp_client_init (tcp_client_config_t const * const init_config, std_error_t * const error);

void tcp_client_check_interrupts ();
void tcp_client_receive_message (tcp_msg_t * const tcp_msg);
int tcp_client_connect (std_error_t * const error);
int tcp_client_send_message (tcp_msg_t const * const tcp_msg, std_error_t * const error);

#endif // TCP_CLIENT_H