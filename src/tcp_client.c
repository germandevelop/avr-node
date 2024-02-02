/************************************************************
 *   Author : German Mundinger
 *   Date   : 2023
 ************************************************************/

#include "tcp_client.h"

#include <string.h>
#include <assert.h>

#include "socket.h"

#include "std_error/std_error.h"
#include "logger.h"

#define W5500_SOCKET_NUMBER 0U

#define FILE_NAME           "tcp_client.c"
#define DEFAULT_ERROR_TEXT  "TCP error"
#define SENDING_ERROR_TEXT  "TCP messg sending error"
#define BUSY_ERROR_TEXT     "TCP busy"

#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


#ifdef NDEBUG
#define TCP_DEBUG(...) ((void)0U)
#else
//static void tcp_client_print_state (char *header);
//#define TCP_DEBUG(...) tcp_client_print_state(__VA_ARGS__)
#define TCP_DEBUG(...) ((void)0U)
#endif // NDEBUG


static tcp_client_config_t config;
static bool is_message_received;
static bool is_connected;


static void tcp_client_spi_select ();
static void tcp_client_spi_unselect ();
static uint8_t tcp_client_spi_read_byte ();
static void tcp_client_spi_write_byte (uint8_t byte);

static int tcp_client_setup_w5500 (std_error_t * const error);

int tcp_client_init (tcp_client_config_t const * const init_config, std_error_t * const error)
{
    assert(init_config                          != NULL);
    assert(init_config->spi_select_callback     != NULL);
    assert(init_config->spi_unselect_callback   != NULL);
    assert(init_config->spi_read_callback       != NULL);
    assert(init_config->spi_write_callback      != NULL);

    memcpy((void*)(&config), (const void*)(init_config), sizeof(tcp_client_config_t));

    is_message_received = false;
    is_connected        = false;

    return tcp_client_setup_w5500(error);
}

void tcp_client_check_interrupts ()
{
    uint8_t interrupt_kind;
    ctlsocket(W5500_SOCKET_NUMBER, CS_GET_INTERRUPT, (void*)(&interrupt_kind));

    uint8_t clear_interrupt = (uint8_t)(SIK_RECEIVED | SIK_DISCONNECTED);
    ctlsocket(W5500_SOCKET_NUMBER, CS_CLR_INTERRUPT, (void*)(&clear_interrupt));

    if ((interrupt_kind & (uint8_t)(SIK_RECEIVED)) != 0U)
    {
        LOG("TCP-SIK_RECEIVED\r\n");

        is_message_received = true;
    }

    if ((interrupt_kind & (uint8_t)(SIK_DISCONNECTED)) != 0U)
    {
        LOG("TCP-SIK_DISCONNECTED\r\n");

        is_connected = false;

        // Disable interrupts
        uint8_t clear_interrupt_mask = 0U;
        ctlsocket(W5500_SOCKET_NUMBER, CS_SET_INTMASK, (void*)(&clear_interrupt_mask));
    }
    return;
}

void tcp_client_receive_message (tcp_msg_t * const tcp_msg)
{
    assert(tcp_msg != NULL);

    tcp_msg->size = 0U;

    if (is_message_received == true)
    {
        tcp_msg->size = (size_t)recv(W5500_SOCKET_NUMBER, (uint8_t*)tcp_msg->buffer, ARRAY_SIZE(tcp_msg->buffer));
        tcp_msg->buffer[tcp_msg->size] = '\0';

        is_message_received = false;
    }
    return;
}

int tcp_client_connect (std_error_t * const error)
{
    const int8_t phy_link = wizphy_getphylink();
    
    if (phy_link != PHY_LINK_ON)
    {
        is_connected = false;

        std_error_catch_custom(error, (int)phy_link, DEFAULT_ERROR_TEXT, FILE_NAME, __LINE__);

        return STD_FAILURE;
    }

    if (is_connected == false)
    {
        TCP_DEBUG("try to disconnect");

        uint8_t socket_status;
        getsockopt(W5500_SOCKET_NUMBER, SO_STATUS, (void*)(&socket_status));

        if (socket_status == SOCK_CLOSE_WAIT)
        {
            const int8_t exit_code = disconnect(W5500_SOCKET_NUMBER);

            if (exit_code != SOCK_OK)
            {
                close(W5500_SOCKET_NUMBER);
            }
        }

        TCP_DEBUG("try to create a socket");

        int8_t exit_code = socket(W5500_SOCKET_NUMBER, Sn_MR_TCP, 0U, 0U);

        if (exit_code != W5500_SOCKET_NUMBER)
        {
            std_error_catch_custom(error, (int)exit_code, DEFAULT_ERROR_TEXT, FILE_NAME, __LINE__);

            return STD_FAILURE;
        }

        TCP_DEBUG("try to connect");

        exit_code = connect(W5500_SOCKET_NUMBER, config.server_ip, config.server_port);

        if (exit_code != SOCK_OK)
        {
            std_error_catch_custom(error, (int)exit_code, DEFAULT_ERROR_TEXT, FILE_NAME, __LINE__);

            return STD_FAILURE;
        }
        else
        {
            uint8_t socket_interrupt_mask = (uint8_t)(SIK_DISCONNECTED | SIK_RECEIVED);
            ctlsocket(W5500_SOCKET_NUMBER, CS_SET_INTMASK, (void*)(&socket_interrupt_mask));

            is_connected = true;
        }
    }

    return STD_SUCCESS;
}

int tcp_client_send_message (tcp_msg_t const * const tcp_msg, std_error_t * const error)
{
    assert(tcp_msg != NULL);

    if (is_connected == false)
    {
        std_error_catch_custom(error, (-1), DEFAULT_ERROR_TEXT, FILE_NAME, __LINE__);

        return STD_FAILURE;
    }

    uint8_t status;
    getsockopt(W5500_SOCKET_NUMBER, SO_STATUS, (void*)(&status));

    if (status == SOCK_SYNSENT)
    {
        std_error_catch_custom(error, (int)status, BUSY_ERROR_TEXT, FILE_NAME, __LINE__);

        return STD_FAILURE;
    }

    const int32_t exit_code = send(W5500_SOCKET_NUMBER, (uint8_t*)tcp_msg->buffer, (uint16_t)tcp_msg->size);

    if (exit_code < SOCK_OK)
    {
        std_error_catch_custom(error, (int)exit_code, SENDING_ERROR_TEXT, FILE_NAME, __LINE__);

        return STD_FAILURE;
    }

    return STD_SUCCESS;
}


int tcp_client_setup_w5500 (std_error_t * const error)
{
    TCP_DEBUG("setup begin");

    reg_wizchip_cs_cbfunc(tcp_client_spi_select, tcp_client_spi_unselect);
    reg_wizchip_spi_cbfunc(tcp_client_spi_read_byte, tcp_client_spi_write_byte);

    uint8_t rx_tx_buffer_sizes[8] = { 0 };
    rx_tx_buffer_sizes[W5500_SOCKET_NUMBER] = 16U;

    int8_t exit_code = wizchip_init(rx_tx_buffer_sizes, rx_tx_buffer_sizes);

    if (exit_code != 0)
    {
        std_error_catch_custom(error, (int)exit_code, DEFAULT_ERROR_TEXT, FILE_NAME, __LINE__);

        return STD_FAILURE;
    }

    wiz_PhyConf phy_config;
    phy_config.by       = PHY_CONFBY_SW;
    phy_config.mode     = PHY_MODE_MANUAL;
    phy_config.duplex   = PHY_DUPLEX_FULL;
    phy_config.speed    = PHY_SPEED_10;

    wizphy_setphyconf(&phy_config);

    wiz_NetTimeout timeout_config;
    timeout_config.time_100us   = 2000U;
    timeout_config.retry_cnt    = 8U;

    wizchip_settimeout(&timeout_config);

    wiz_NetInfo net_info;
    memcpy((void*)(net_info.mac), (const void*)(config.mac_address), sizeof(net_info.mac));
    memcpy((void*)(net_info.ip), (const void*)(config.ip_address), sizeof(net_info.ip));
    memcpy((void*)(net_info.sn), (const void*)(config.netmask), sizeof(net_info.sn));
    net_info.dhcp = NETINFO_STATIC;

    wizchip_setnetinfo(&net_info);

    wizchip_setinterruptmask(IK_SOCK_0);

    //wizchip_setnetmode(netmode_type netmode); // Unknown
    //wizphy_setphypmode(PHY_POWER_DOWN);

    TCP_DEBUG("setup end");

    return STD_SUCCESS;
}

void tcp_client_spi_select ()
{
    config.spi_select_callback();

    return;
}

void tcp_client_spi_unselect ()
{
    config.spi_unselect_callback();

    return;
}

uint8_t tcp_client_spi_read_byte ()
{
    uint8_t byte;

    config.spi_read_callback(&byte);

    return byte;
}

void tcp_client_spi_write_byte (uint8_t byte)
{
    config.spi_write_callback(byte);

    return;
}


#ifdef NDEBUG
void tcp_client_print_state (char *header)
{
    LOG("\r\n___W5500 - %s\r\n", header);

    int8_t phylink = wizphy_getphylink();
    int8_t phymode = wizphy_getphypmode();

    LOG("link: %i | mode: %i\r\n", phylink, phymode);

    wiz_PhyConf phyconf;
    wizphy_getphyconf(&phyconf);

    LOG("by: %u | duplex: %u | mode: %u | speed: %u\r\n", phyconf.by, phyconf.duplex, phyconf.mode, phyconf.speed);

    wizphy_getphystat(&phyconf);

    LOG("duplex: %u | speed: %u\r\n", phyconf.duplex, phyconf.speed);

    netmode_type type = wizchip_getnetmode();

    LOG("type: %i\r\n", type);
    LOG("NM_FORCEARP - %i; NM_WAKEONLAN - %i; NM_PINGBLOCK - %i; NM_PPPOE - %i\r\n", NM_FORCEARP, NM_WAKEONLAN, NM_PINGBLOCK, NM_PPPOE);

    wiz_NetTimeout nettime;
    wizchip_gettimeout(&nettime);

    LOG("retry: %u | time: %u\r\n", nettime.retry_cnt, nettime.time_100us);

    intr_kind mask_low = wizchip_getinterruptmask();

    LOG("interrupt mask: %i\r\n", mask_low);

    uint8_t mask;
    ctlsocket(W5500_SOCKET_NUMBER, CS_GET_INTMASK, (void*)&mask);

    LOG("socket int mask: %u\r\n", mask);
    LOG("SIK_CONNECTED - %u; SIK_DISCONNECTED - %u; SIK_RECEIVED - %u\r\n", SIK_CONNECTED, SIK_DISCONNECTED, SIK_RECEIVED);
    LOG("SIK_TIMEOUT - %u; SIK_SENT - %u; SIK_ALL - %u\r\n", SIK_TIMEOUT, SIK_SENT, SIK_ALL);
        
    uint8_t status;
    getsockopt(W5500_SOCKET_NUMBER, SO_STATUS, (void*)(&status));

    if (status == SOCK_CLOSED) LOG("status: SOCK_CLOSED\r\n");
    else if (status == SOCK_INIT) LOG("status: SOCK_INIT\r\n");
    else if (status == SOCK_LISTEN) LOG("status: SOCK_LISTEN\r\n");
    else if (status == SOCK_SYNSENT) LOG("status: SOCK_SYNSENT\r\n");
    else if (status == SOCK_SYNRECV) LOG("status: SOCK_SYNRECV\r\n");
    else if (status == SOCK_ESTABLISHED) LOG("status: SOCK_ESTABLISHED\r\n");
    else if (status == SOCK_FIN_WAIT) LOG("status: SOCK_FIN_WAIT\r\n");
    else if (status == SOCK_CLOSING) LOG("status: SOCK_CLOSING\r\n");
    else if (status == SOCK_TIME_WAIT) LOG("status: SOCK_TIME_WAIT\r\n");
    else if (status == SOCK_CLOSE_WAIT) LOG("status: SOCK_CLOSE_WAIT\r\n");
    else if (status == SOCK_LAST_ACK) LOG("status: SOCK_LAST_ACK\r\n");
    else if (status == SOCK_UDP) LOG("status: SOCK_UDP\r\n");
    else if (status == SOCK_IPRAW) LOG("status: SOCK_IPRAW\r\n");
    else if (status == SOCK_MACRAW) LOG("status: SOCK_MACRAW\r\n");

    return;
}
#endif // NDEBUG
