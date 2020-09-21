/*
  srvspi_drv.h - Library for Arduino Wifi SPI connection with ESP8266.
  Copyright (c) 2017 Jiri Bilek. All right reserved.

  ---

  Based on server_drv.h - Library for Arduino Wifi shield.
  Copyright (c) 2011-2014 Arduino.  All right reserved.

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

#ifndef _SRVSPI_DRV_H_INCLUDED
#define _SRVSPI_DRV_H_INCLUDED

#include <inttypes.h>
#include "wifi_spi.h"
#include <stdbool.h>

typedef enum eProtMode
{
  TCP_MODE,
  UDP_MODE,
  TCP_MODE_WITH_TLS
} tProtMode_t;

bool ServerSpiDrv_startServer(uint16_t port, uint8_t sock, uint8_t protMode);
void ServerSpiDrv_stopServer(uint8_t sock);
bool ServerSpiDrv_startClient(uint32_t ipAddress, uint16_t port, uint8_t sock, uint8_t protMode);
void ServerSpiDrv_stopClient(uint8_t sock);
uint8_t ServerSpiDrv_getServerState(uint8_t sock);
uint8_t ServerSpiDrv_getClientState(const uint8_t sock);
uint16_t ServerSpiDrv_availData(const uint8_t sock);
bool ServerSpiDrv_getData(const uint8_t sock, int16_t *data, const uint8_t peek);
bool ServerSpiDrv_getDataBuf(const uint8_t sock, uint8_t *_data, uint16_t *_dataLen);
bool ServerSpiDrv_insertDataBuf(uint8_t sock, const uint8_t *data, uint16_t _len);
bool ServerSpiDrv_sendUdpData(uint8_t sock);
bool ServerSpiDrv_sendData(const uint8_t sock, const uint8_t *data, const uint16_t len);
bool ServerSpiDrv_beginUdpPacket(uint32_t ipAddress, uint16_t port, uint8_t sock);
uint16_t ServerSpiDrv_parsePacket(const uint8_t sock);
uint8_t ServerSpiDrv_verifySSLClient(const uint8_t sock, uint8_t *fingerprint, const char *host);

#endif
