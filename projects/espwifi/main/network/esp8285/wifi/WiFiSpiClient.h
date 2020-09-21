/*
  WiFiSpiClient.h - Library for Arduino with ESP8266 as slave.
  Copyright (c) 2017 Jiri Bilek. All right reserved.

  ---

  Based on WiFiClient.h - Library for Arduino Wifi shield.
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

#ifndef _WIFISPICLIENT_H_INCLUDED
#define _WIFISPICLIENT_H_INCLUDED

#include <stdio.h>

#include "printf.h"

#include "wifispi_drv.h"

uint8_t WiFiSpiClient_connect_host(const char *host, uint16_t port,uint8_t isSSL);
uint8_t WiFiSpiClient_connect_ip(IPAddress_t ip, uint16_t port,uint8_t isSSL);

size_t WiFiSpiClient_write(uint8_t sock, const uint8_t *buf, size_t size);
int WiFiSpiClient_available(uint8_t sock);
int WiFiSpiClient_read_oneByte(uint8_t sock);
int WiFiSpiClient_read_buf(uint8_t sock, uint8_t *buf, size_t size);
int WiFiSpiClient_peek(uint8_t sock);
void WiFiSpiClient_flush(uint8_t sock);
void WiFiSpiClient_stop(uint8_t sock);
uint8_t WiFiSpiClient_connected(uint8_t sock);
uint8_t WiFiSpiClient_status(uint8_t sock);
uint8_t WiFiSpiClient_verifySSL(uint8_t sock, uint8_t *fingerprint, const char *host);
#endif
