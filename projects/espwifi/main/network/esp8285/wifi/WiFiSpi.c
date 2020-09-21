/*
  WiFiSPI.cpp - Library for Arduino SPI connection to ESP8266
  Copyright (c) 2017 Jiri Bilek. All rights reserved.

  -----

  Based on WiFi.cpp - Library for Arduino Wifi shield.
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

#include "WiFiSpi.h"
#include "utility/wifispi_drv.h"

#include "utility/wl_definitions.h"
#include "utility/wl_types.h"
///  #include "utility/debug.h"
#include "sleep.h"

#include "espspi_proxy.h"
#include "gpiohs.h"

// Protocol version
const char *WiFiSpi_protocolVer = "0.2.3";

// Hardware reset pin

int8_t WiFiSpi_hwResetPin = -1;
// No assumptions about the value of MAX_SOCK_NUM
int16_t WiFiSpi_state[MAX_SOCK_NUM];
uint16_t WiFiSpi_server_port[MAX_SOCK_NUM];

void WiFiSpi_init(int8_t rst_pin)
{
    // Initialize the connection arrays
    for(uint8_t i = 0; i < MAX_SOCK_NUM; ++i)
    {
        WiFiSpi_state[i] = NA_STATE;
        WiFiSpi_server_port[i] = 0;
    }

    WiFiSpi_hwResetPin = rst_pin;

    WiFiSpiDrv_wifiDriverInit();
    WiFiSpi_hardReset();
}

uint8_t WiFiSpi_getSocket()
{
    for(uint8_t i = 0; i < MAX_SOCK_NUM; ++i)
    {
        if(WiFiSpi_state[i] == NA_STATE) // _state is for both server and client
            return i;
    }
    return SOCK_NOT_AVAIL;
}

/*
 *
 */
char *WiFiSpi_firmwareVersion()
{
    return WiFiSpiDrv_getFwVersion();
}

/*
 *
 */
uint8_t WiFiSpi_begin_ssid(const char *ssid)
{
    uint8_t status = WL_IDLE_STATUS;
    uint8_t attempts = WL_MAX_ATTEMPT_CONNECTION;

    if(WiFiSpiDrv_wifiSetNetwork(ssid, strlen(ssid)) != WL_FAILURE)
    {
        // TODO: Improve timing
        do
        {
            msleep(WL_DELAY_START_CONNECTION);
            status = WiFiSpiDrv_getConnectionStatus();
        } while(((status == WL_IDLE_STATUS) || (status == WL_SCAN_COMPLETED) || (status == WL_DISCONNECTED)) && (--attempts > 0));
    } else
        status = WL_CONNECT_FAILED;

    return status;
}

/*///uint8_t WiFiSpi_begin(const char* ssid, uint8_t key_idx, const char *key)
{
	uint8_t status = WL_IDLE_STATUS;
	uint8_t attempts = WL_MAX_ATTEMPT_CONNECTION;

	// set encryption key
    if (WiFiSpiDrv_wifiSetKey(ssid, strlen(ssid), key_idx, key, strlen(key)) != WL_FAILURE)
    {
        //
	    do
	    {
		    delay(WL_DELAY_START_CONNECTION);
		    status = WiFiSpiDrv_getConnectionStatus();
	    }
        while (((status == WL_IDLE_STATUS) || (status == WL_SCAN_COMPLETED) || (status == WL_DISCONNECTED)) && (--attempts > 0));
   } else {
	    status = WL_CONNECT_FAILED;
   }

   return status;
}*/

/*
 *
 */
uint8_t WiFiSpi_begin_ssid_passwd(const char *ssid, const char *passphrase)
{
    uint8_t status = WL_IDLE_STATUS;
    uint8_t attempts = WL_MAX_ATTEMPT_CONNECTION; //100*100ms=10s

    // SSID and passphrase for WPA connection
    if(WiFiSpiDrv_wifiSetPassphrase(ssid, strlen(ssid), passphrase, strlen(passphrase)) != WL_FAILURE)
    {
        // TODO: Improve timing
        do
        {
            msleep(WL_DELAY_START_CONNECTION);
            status = WiFiSpiDrv_getConnectionStatus();
        } while(((status == WL_IDLE_STATUS) || (status == WL_SCAN_COMPLETED) || (status == WL_DISCONNECTED)) && (--attempts > 0));
    } else
        status = WL_CONNECT_FAILED;

    return status;
}

/*
 *
 */
bool WiFiSpi_config_ip(IPAddress_t local_ip)
{
    return WiFiSpiDrv_config(IPAddress_to_uint32(&local_ip), 0, 0, 0, 0);
}

/*
 *
 */
bool WiFiSpi_config_ip_dns(IPAddress_t local_ip, IPAddress_t dns_server)
{
    return WiFiSpiDrv_config(IPAddress_to_uint32(&local_ip), 0, 0, IPAddress_to_uint32(&dns_server), 0);
}

/*
 *
 */
bool WiFiSpi_config_ip_dns_gw(IPAddress_t local_ip, IPAddress_t dns_server, IPAddress_t gateway)
{
    return WiFiSpiDrv_config(IPAddress_to_uint32(&local_ip), IPAddress_to_uint32(&gateway), 0, IPAddress_to_uint32(&dns_server), 0);
}

/*
 *
 */
bool WiFiSpi_config_ip_dns_gw_sn(IPAddress_t local_ip, IPAddress_t dns_server, IPAddress_t gateway, IPAddress_t subnet)
{
    return WiFiSpiDrv_config(IPAddress_to_uint32(&local_ip), IPAddress_to_uint32(&gateway), IPAddress_to_uint32(&subnet), IPAddress_to_uint32(&dns_server), 0);
}

/*
 *
 */
bool WiFiSpi_setDNS_1(IPAddress_t dns_server1)
{
    return WiFiSpiDrv_config(0, 0, 0, IPAddress_to_uint32(&dns_server1), 0);
}

/*
 *
 */
bool WiFiSpi_setDNS_2(IPAddress_t dns_server1, IPAddress_t dns_server2)
{
    return WiFiSpiDrv_config(0, 0, 0, IPAddress_to_uint32(&dns_server1), IPAddress_to_uint32(&dns_server2));
}

/*
 *
 */
int WiFiSpi_disconnect()
{
    return WiFiSpiDrv_disconnect();
}

/*
 *
 */
uint8_t *WiFiSpi_macAddress(uint8_t *mac)
{
    uint8_t *_mac = WiFiSpiDrv_getMacAddress();
    memcpy(mac, _mac, WL_MAC_ADDR_LENGTH);
    return mac;
}

/*
 *
 */
IPAddress_t WiFiSpi_localIP()
{
    IPAddress_t ret;
    WiFiSpiDrv_getIpAddress(&ret);
    return ret;
}

/*
 *
 */
IPAddress_t WiFiSpi_subnetMask()
{
    IPAddress_t ret;
    WiFiSpiDrv_getSubnetMask(&ret);
    return ret;
}

/*
 *
 */
IPAddress_t WiFiSpi_gatewayIP()
{
    IPAddress_t ret;
    WiFiSpiDrv_getGatewayIP(&ret);
    return ret;
}

/*
 *
 */
char *WiFiSpi_SSID()
{
    return WiFiSpiDrv_getCurrentSSID();
}

/*
 *
 */
uint8_t *WiFiSpi_BSSID()
{
    return WiFiSpiDrv_getCurrentBSSID();
}

/*
 *
 */
int32_t WiFiSpi_RSSI()
{
    return WiFiSpiDrv_getCurrentRSSI();
}

/*
uint8_t WiFiSpi_encryptionType()
{
    return WiFiSpiDrv_getCurrentEncryptionType();
}
*/

/*
 *
 */
int8_t WiFiSpi_scanNetworks()
{
#define WIFI_SCAN_RUNNING (-1)
#define WIFI_SCAN_FAILED (-2)

    uint8_t attempts = 10;
    int8_t numOfNetworks = 0;

    if(WiFiSpiDrv_startScanNetworks() == WIFI_SCAN_FAILED)
        return WL_FAILURE;

    do
    {
        sleep(2000);
        numOfNetworks = WiFiSpiDrv_getScanNetworks();

        if(numOfNetworks == WIFI_SCAN_FAILED)
            return WL_FAILURE;
    } while((numOfNetworks == WIFI_SCAN_RUNNING) && (--attempts > 0));

    return numOfNetworks;
}

/*
 *
 */
char *WiFiSpi_SSID_item(uint8_t networkItem)
{
    return WiFiSpiDrv_getSSIDNetworks(networkItem);
}

/*
 *
 */
int32_t WiFiSpi_RSSI_item(uint8_t networkItem)
{
    return WiFiSpiDrv_getRSSINetworks(networkItem);
}

/*
 *
 */
uint8_t WiFiSpi_encryptionType_item(uint8_t networkItem)
{
    return WiFiSpiDrv_getEncTypeNetworks(networkItem);
}

/*
 *
 */
uint8_t WiFiSpi_status()
{
    return WiFiSpiDrv_getConnectionStatus();
}

/*
 *
 */
int8_t WiFiSpi_hostByName(const char *aHostname, IPAddress_t *aResult)
{
    return WiFiSpiDrv_getHostByName(aHostname, aResult);
}

/*
 * Perform remote software reset of the ESP8266 module.
 * The reset succeedes only if the SPI communication is not broken.
 * The function does not wait for the ESP8266.
 */
void WiFiSpi_softReset(void)
{
    WiFiSpiDrv_softReset();
}

/*
 *
 */
char *WiFiSpi_protocolVersion()
{
    return WiFiSpiDrv_getProtocolVersion();
}

/*
 *
 */
const char *WiFiSpi_masterProtocolVersion()
{
    return WiFiSpi_protocolVer;
}

/*
 *
 */
uint8_t WiFiSpi_checkProtocolVersion()
{
    const char *s = WiFiSpiDrv_getProtocolVersion();
    for(const char *p = WiFiSpi_protocolVer; *p; ++p, ++s)
        if(*p != *s)
            return 0;

    return (*s == 0);
}

/*
 *
 */
void WiFiSpi_hardReset(void)
{
    if(WiFiSpi_hwResetPin <= 0)
        return; // no reset pin

    gpiohs_set_drive_mode(WiFiSpi_hwResetPin, GPIO_DM_OUTPUT);
    EspSpiProxy_hardReset(WiFiSpi_hwResetPin);
}
