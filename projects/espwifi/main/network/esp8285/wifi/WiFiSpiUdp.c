/*
  WiFiSPIUdp.cpp - Library for Arduino Wifi SPI connection with ESP8266.
  Copyright (c) 2017 Jiri Bilek. All right reserved.

  ---

  Based on WiFiUdp.cpp - Library for Arduino Wifi shield.
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

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "wifi_spi.h"
#include "srvspi_drv.h"
#include "wifispi_drv.h"

#include "WiFiSpi.h"
#include "WiFiSpiUdp.h"

// static uint8_t _sock = SOCK_NOT_AVAIL;
static uint16_t _port;

/*
 * Starts WiFiSpiUdp socket, listening at local port PORT
 */
uint8_t WiFiSpiUdp_begin(uint16_t port)
{
    uint8_t sock = WiFiSpi_getSocket();
    if (sock != SOCK_NOT_AVAIL)
    {
        if (ServerSpiDrv_startServer(port, sock, UDP_MODE))
        {
            WiFiSpi_server_port[sock] = port;
            WiFiSpi_state[sock] = sock;
            _port = port;
            return sock;
        }
    }
    return SOCK_NOT_AVAIL;
}

/*
 *  Returns number of bytes available in the current packet
 */
int WiFiSpiUdp_available(uint8_t sock)
{
    if (sock != SOCK_NOT_AVAIL)
        return ServerSpiDrv_availData(sock);

    return 0;
}

/*
 * Releases any resources being used by this WiFiSpiUdp instance
 */
void WiFiSpiUdp_stop(uint8_t sock)
{
    ServerSpiDrv_stopServer(sock);

    WiFiSpi_server_port[sock] = 0;
    WiFiSpi_state[sock] = NA_STATE;
}

/*
 *
 */
int WiFiSpiUdp_beginPacket(uint8_t sock, const char *host, uint16_t port)
{
    // Look up the host first
    IPAddress_t remote_addr;
    if (WiFiSpi_hostByName(host, &remote_addr))
        return WiFiSpiUdp_beginPacket_ipaddr(sock, remote_addr, port);
    else
        return 0;
}

/*
 *
 */
int WiFiSpiUdp_beginPacket_ipaddr(uint8_t sock, IPAddress_t ip, uint16_t port)
{
    if (sock == SOCK_NOT_AVAIL)
        return 0; // No socket available

    if (!ServerSpiDrv_beginUdpPacket(IPAddress_to_uint32(&ip), port, sock))
        return 0; // Client not opened
    else
        return 1;
}

/*
 *
 */
int WiFiSpiUdp_endPacket(uint8_t sock)
{
    return ServerSpiDrv_sendUdpData(sock);
}

/*
 *
 */
size_t WiFiSpiUdp_write_oneByte(uint8_t sock, uint8_t byte)
{
    return WiFiSpiUdp_write_buf(sock, &byte, 1);
}

/*
 *
 */
size_t WiFiSpiUdp_write_buf(uint8_t sock, const uint8_t *buffer, size_t size)
{
    if (ServerSpiDrv_insertDataBuf(sock, buffer, size))
        return size;
    else
        return 0;
}

/*
 *
 */
int WiFiSpiUdp_parsePacket(uint8_t sock)
{
    return ServerSpiDrv_parsePacket(sock);
}

/*
 *
 */
int WiFiSpiUdp_read_oneByte(uint8_t sock)
{
    int16_t b;
    ServerSpiDrv_getData(sock, &b, 0); // returns -1 when error
    return b;
}

/*
 *
 */
int WiFiSpiUdp_read_buf(uint8_t sock, unsigned char *buffer, size_t len)
{
    // sizeof(size_t) is architecture dependent
    // but we need a 16 bit data type here
    uint16_t _size = len;

    if (!ServerSpiDrv_getDataBuf(sock, buffer, &_size))
        return -1;
    else
        return 0;
}

/*
 *
 */
int WiFiSpiUdp_peek(uint8_t sock)
{
    int16_t b;
    ServerSpiDrv_getData(sock, &b, 1); // returns -1 when error
    return b;
}

/*
 *
 */
void WiFiSpiUdp_flush(uint8_t sock)
{
    // TODO: a real check to ensure transmission has been completed
}

/*
 *
 */
IPAddress_t WiFiSpiUdp_remoteIP(uint8_t sock)
{
    uint8_t _remoteIp[4];
    uint16_t _remotePort;

    if (WiFiSpiDrv_getRemoteData(sock, _remoteIp, &_remotePort))
        return uint8_to_IPAddress(_remoteIp);
    else
    {
        _remoteIp[0] = 0;
        _remoteIp[1] = 0;
        _remoteIp[2] = 0;
        _remoteIp[3] = 0;
        return uint8_to_IPAddress(_remoteIp);
    }
}

/*
 *
 */
uint16_t WiFiSpiUdp_remotePort(uint8_t sock)
{
    uint8_t _remoteIp[4];
    uint16_t _remotePort;

    if (WiFiSpiDrv_getRemoteData(sock, _remoteIp, &_remotePort))
        return _remotePort;
    else
        return 0;
}
