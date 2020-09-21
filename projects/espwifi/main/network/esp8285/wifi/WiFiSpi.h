/*
  WiFiSPI.h - Library for Arduino SPI connection to ESP8266
  Copyright (c) 2017 Jiri Bilek. All rights reserved.

 Circuit:
   1. On ESP8266 must be running (flashed) WiFiSPIESP application.

   2. Connect the Arduino to the following pins on the esp8266:

            ESP8266         |
    GPIO    NodeMCU   Name  |   Uno
   ===================================
     15       D8       SS   |   D10
     13       D7      MOSI  |   D11
     12       D6      MISO  |   D12
     14       D5      SCK   |   D13

    Note: If the ESP is booting at a moment when the SPI Master (i.e. Arduino) has the Select line HIGH (deselected)
    the ESP8266 WILL FAIL to boot!

  -----

  Based on WiFi.h - Library for Arduino Wifi shield.
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

///////////////////////////////////////////////////////////////////////////////
//this lib commit id e1f86c21b3d8bcd87002a33abf903c139c2f572d
///////////////////////////////////////////////////////////////////////////////

#ifndef _WIFISPI_H_INCLUDED
#define _WIFISPI_H_INCLUDED

#include <inttypes.h>
#include "utility/wl_definitions.h"
#include "utility/wl_types.h"
#include "utility/wifi_spi.h"
#include "WiFiSpiClient.h"
#include "WiFiSpiServer.h"
#include "WiFiSpiUdp.h"
#include "utility/wifispi_drv.h"

extern int16_t WiFiSpi_state[MAX_SOCK_NUM];
extern uint16_t WiFiSpi_server_port[MAX_SOCK_NUM];

void WiFiSpi_init(int8_t rst_pin);
uint8_t WiFiSpi_getSocket();
char *WiFiSpi_firmwareVersion();

uint8_t WiFiSpi_begin_ssid(const char *ssid);
uint8_t WiFiSpi_begin_ssid_passwd(const char *ssid, const char *passphrase);

bool WiFiSpi_config_ip(IPAddress_t local_ip);
bool WiFiSpi_config_ip_dns(IPAddress_t local_ip, IPAddress_t dns_server);
bool WiFiSpi_config_ip_dns_gw(IPAddress_t local_ip, IPAddress_t dns_server, IPAddress_t gateway);
bool WiFiSpi_config_ip_dns_gw_sn(IPAddress_t local_ip, IPAddress_t dns_server, IPAddress_t gateway, IPAddress_t subnet);

bool WiFiSpi_setDNS_1(IPAddress_t dns_server1);
bool WiFiSpi_setDNS_2(IPAddress_t dns_server1, IPAddress_t dns_server2);

int WiFiSpi_disconnect();

uint8_t *WiFiSpi_macAddress(uint8_t *mac);
IPAddress_t WiFiSpi_localIP();
IPAddress_t WiFiSpi_subnetMask();
IPAddress_t WiFiSpi_gatewayIP();

char *WiFiSpi_SSID();
uint8_t *WiFiSpi_BSSID();
int32_t WiFiSpi_RSSI();
// uint8_t WiFiSpi_encryptionType();

int8_t WiFiSpi_scanNetworks();

char *WiFiSpi_SSID_item(uint8_t networkItem);
int32_t WiFiSpi_RSSI_item(uint8_t networkItem);
uint8_t WiFiSpi_encryptionType_item(uint8_t networkItem);

uint8_t WiFiSpi_status();

int8_t WiFiSpi_hostByName(const char *aHostname, IPAddress_t *aResult);
void WiFiSpi_softReset(void);
void WiFiSpi_hardReset(void);
char *WiFiSpi_protocolVersion();
const char *WiFiSpi_masterProtocolVersion();
uint8_t WiFiSpi_checkProtocolVersion();

#endif
