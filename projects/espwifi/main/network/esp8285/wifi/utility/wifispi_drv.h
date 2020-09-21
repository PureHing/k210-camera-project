/*
  wifispi_drv.h - Library for Arduino SPI connection to ESP8266
  Copyright (c) 2017 Jiri Bilek. All right reserved.

  ---

  Based on: wifi_drv.h - Library for Arduino Wifi shield.
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

#ifndef _WIFISPI_DRV_H_INCLUDED
#define _WIFISPI_DRV_H_INCLUDED

#include <inttypes.h>
#include "wifi_spi.h"
#include <stdbool.h>

/* clang-format off */
// Key index length
///#define KEY_IDX_LEN              1
// 100 ms secs of delay to test if the connection is established
#define WL_DELAY_START_CONNECTION 100
// Firmware version string length (format a.b.c)
#define WL_FW_VER_LENGTH            6
// Protocol version string length (format a.b.c)
#define WL_PROTOCOL_VER_LENGTH      6

#define DUMMY_DATA                  0xFF
/* clang-format on */

typedef struct
{
  uint8_t address[WL_IPV4_LENGTH];
} IPAddress_t;

bool WiFiSpiDrv_getRemoteData(uint8_t sock, uint8_t *ip, uint16_t *port);

void WiFiSpiDrv_wifiDriverInit(void);
int8_t WiFiSpiDrv_wifiSetNetwork(const char *ssid, uint8_t ssid_len);
int8_t WiFiSpiDrv_wifiSetPassphrase(const char *ssid, const uint8_t ssid_len, const char *passphrase, const uint8_t len);
bool WiFiSpiDrv_config(uint32_t local_ip, uint32_t gateway, uint32_t subnet, uint32_t dns_server1, uint32_t dns_server2);
uint8_t WiFiSpiDrv_disconnect();
uint8_t WiFiSpiDrv_getConnectionStatus();
uint8_t *WiFiSpiDrv_getMacAddress();
int8_t WiFiSpiDrv_getIpAddress(IPAddress_t *ip);
int8_t WiFiSpiDrv_getSubnetMask(IPAddress_t *mask);
int8_t WiFiSpiDrv_getGatewayIP(IPAddress_t *ip);
char *WiFiSpiDrv_getCurrentSSID();
uint8_t *WiFiSpiDrv_getCurrentBSSID();
int32_t WiFiSpiDrv_getCurrentRSSI();
int8_t WiFiSpiDrv_startScanNetworks();
int8_t WiFiSpiDrv_getScanNetworks();
char *WiFiSpiDrv_getSSIDNetworks(uint8_t networkItem);
uint8_t WiFiSpiDrv_getEncTypeNetworks(uint8_t networkItem);
int32_t WiFiSpiDrv_getRSSINetworks(uint8_t networkItem);
int8_t WiFiSpiDrv_getHostByName(const char *aHostname, IPAddress_t *aResult);
char *WiFiSpiDrv_getFwVersion();
void WiFiSpiDrv_softReset(void);
char *WiFiSpiDrv_getProtocolVersion();

uint32_t IPAddress_to_uint32(IPAddress_t *ip);
IPAddress_t uint8_to_IPAddress(uint8_t *ip);
char *IPAddress_to_string(IPAddress_t *ip);

#endif
