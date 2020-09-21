/*
  WiFiSpiClient.cpp - Library for Arduino SPI connection to ESP8266
  Copyright (c) 2017 Jiri Bilek. All rights reserved.

  -----

  Based on WiFiClient.cpp - Library for Arduino Wifi shield.
  Copyright (c) 2011-2014 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <stdbool.h>
#include <stdio.h>

#include "wl_definitions.h"
#include "wl_types.h"
#include "WiFiSpi.h"
#include "WiFiSpiClient.h"
#include "wifi_spi.h"
#include "srvspi_drv.h"
#include "wifispi_drv.h"
#include "sleep.h"

// static uint8_t _sock; //not used

/*
 * Connect, return a sock,
 * SOCK_NOT_AVAIL is not ok
 */
uint8_t WiFiSpiClient_connect_host(const char *host, uint16_t port, uint8_t isSSL)
{
    IPAddress_t remote_addr;

    if (WiFiSpi_hostByName(host, &remote_addr))
    {
        return WiFiSpiClient_connect_ip(remote_addr, port, isSSL);
    }
    return SOCK_NOT_AVAIL;
}

/*
 *
 */
uint8_t WiFiSpiClient_connect_ip(IPAddress_t ip, uint16_t port, uint8_t isSSL)
{
    uint8_t sock = WiFiSpi_getSocket();
    if (sock != SOCK_NOT_AVAIL)
    {
        if (!ServerSpiDrv_startClient(IPAddress_to_uint32(&ip), port, sock, (isSSL ? TCP_MODE_WITH_TLS : TCP_MODE)))
            return SOCK_NOT_AVAIL; // unsuccessfull

        WiFiSpi_state[sock] = sock;
    }
    else
    {
        printk("No Socket available");
        return SOCK_NOT_AVAIL;
    }
    return sock;
}

// /*
//  *
//  */
// size_t WiFiSpiClient_write(uint8_t b)
// {
//     return write(&b, 1);
// }

/*
 *
 */
size_t WiFiSpiClient_write(uint8_t sock, const uint8_t *buf, size_t size)
{
    if (sock >= MAX_SOCK_NUM || size == 0)
    {
        // setWriteError();
        printk("%d write error!\r\n",__LINE__);
        return 0;
    }

    if (!ServerSpiDrv_sendData(sock, buf, size))
    {
        // setWriteError();
        printk("%d write error!\r\n",__LINE__);
        return 0;
    }

    return size;
}

/*
 *
 */
int WiFiSpiClient_available(uint8_t sock)
{
    if (sock == SOCK_NOT_AVAIL)
        return 0;
    else
        return ServerSpiDrv_availData(sock);
}

/*
 *
 */
int WiFiSpiClient_read_oneByte(uint8_t sock)
{
    int16_t b;
    ServerSpiDrv_getData(sock, &b, 0); // returns -1 when error
    return b;
}

/*
    Reads data into a buffer.
    Return: 0 = success, size bytes read
           -1 = error (either no data or communication error)
 */
int WiFiSpiClient_read_buf(uint8_t sock, uint8_t *buf, size_t size)
{
    // sizeof(size_t) is architecture dependent
    // but we need a 16 bit data type here
    uint16_t _size = size;
    if (!ServerSpiDrv_getDataBuf(sock, buf, &_size))
        return -1;
    return 0;
}

/*
 *
 */
int WiFiSpiClient_peek(uint8_t sock)
{
    int16_t b;
    ServerSpiDrv_getData(sock, &b, 1); // returns -1 when error
    return b;
}

/*
 *
 */
void WiFiSpiClient_flush(uint8_t sock)
{
    // TODO: a real check to ensure transmission has been completed
}

/*
 *
 */
void WiFiSpiClient_stop(uint8_t sock)
{
    if (sock == SOCK_NOT_AVAIL)
        return;

    ServerSpiDrv_stopClient(sock);

    int count = 0;
    // wait maximum 5 secs for the connection to close
    while (WiFiSpiClient_status(sock) != CLOSED && ++count < 500)
        sleep(1);

    WiFiSpi_state[sock] = NA_STATE;
    sock = SOCK_NOT_AVAIL;
}

/*
 *
 */
uint8_t WiFiSpiClient_connected(uint8_t sock)
{
    if (sock == SOCK_NOT_AVAIL)
        return 0;
    else
        return (WiFiSpiClient_status(sock) == ESTABLISHED);
}

/*
 *
 */
uint8_t WiFiSpiClient_status(uint8_t sock)
{
    if (sock == SOCK_NOT_AVAIL)
        return CLOSED;
    else
        return ServerSpiDrv_getClientState(sock);
}

// /*
//  *
//  */
// WiFiSpiClient_operator bool()
// {
//     return (_sock != SOCK_NOT_AVAIL);
// }

/*
 * 
 */
uint8_t WiFiSpiClient_verifySSL(uint8_t sock, uint8_t *fingerprint, const char *host)
{
    if (sock == SOCK_NOT_AVAIL || host[0] == 0)
        return 0;

    return ServerSpiDrv_verifySSLClient(sock, fingerprint, host);
}
