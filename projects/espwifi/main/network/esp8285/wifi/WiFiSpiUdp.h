/*
  WiFiSPIUdp.h - Library for Arduino Wifi SPI connection with ESP8266.
  Copyright (c) 2017 Jiri Bilek. All right reserved.

  ---

  Based on WiFiUdp.h - Library for Arduino Wifi shield.
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

#ifndef _WIFISPIUDP_H_INCLUDED
#define _WIFISPIUDP_H_INCLUDED

#include <stdio.h>
#include <stdbool.h>
#include "wifispi_drv.h"

#define UDP_TX_PACKET_MAX_SIZE 24

uint8_t WiFiSpiUdp_begin(uint16_t port);
int WiFiSpiUdp_available(uint8_t sock);
void WiFiSpiUdp_stop(uint8_t sock);
int WiFiSpiUdp_beginPacket(uint8_t sock, const char *host, uint16_t port);
int WiFiSpiUdp_beginPacket_ipaddr(uint8_t sock, IPAddress_t ip, uint16_t port);
int WiFiSpiUdp_endPacket(uint8_t sock);
size_t WiFiSpiUdp_write_oneByte(uint8_t sock, uint8_t byte);
size_t WiFiSpiUdp_write_buf(uint8_t sock, const uint8_t *buffer, size_t size);
int WiFiSpiUdp_parsePacket(uint8_t sock);
int WiFiSpiUdp_read_oneByte(uint8_t sock);
int WiFiSpiUdp_read_buf(uint8_t sock, unsigned char *buffer, size_t len);
int WiFiSpiUdp_peek(uint8_t sock);
void WiFiSpiUdp_flush(uint8_t sock);
IPAddress_t WiFiSpiUdp_remoteIP(uint8_t sock);
uint16_t WiFiSpiUdp_remotePort(uint8_t sock);

#endif
