/*
  wifispi_drv.cpp - Library for Arduino SPI connection to ESP8266
  Copyright (c) 2017 Jiri Bilek. All right reserved.

  ---

  Based on wifi_drv.cpp - Library for Arduino Wifi shield.
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

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "espspi_proxy.h"
#include "espspi_drv.h"
#include "wifispi_drv.h"
#include "wifi_spi.h"

#include "wl_types.h"

// Array of data to cache the information related to the networks discovered
char _networkSsid[33] = {0};
int32_t _networkRssi = 0;
uint8_t _networkEncr = 0;

//we do not care memory use

// Cached values of retrieved data
char _ssid[33] = {0};
uint8_t _bssid[32] = {0};
uint8_t _mac[32] = {0};
uint8_t _localIp[32] = {0};
uint8_t _subnetMask[32] = {0};
uint8_t _gatewayIp[32] = {0};

// Firmware and protocol version
char fwVersion[32] = {0};
char protocolVersion[32] = {0};

/*
 *
 */
static int8_t WiFiSpiDrv_getNetworkData(uint8_t *ip, uint8_t *mask, uint8_t *gwip)
{
    tParam_t params[PARAM_NUMS_3] = {{WL_IPV4_LENGTH, (char *)ip}, {WL_IPV4_LENGTH, (char *)mask}, {WL_IPV4_LENGTH, (char *)gwip}};

    // Send Command
    EspSpiDrv_sendCmd(GET_IPADDR_CMD, PARAM_NUMS_0);

    if (!EspSpiDrv_waitResponseParams(GET_IPADDR_CMD, PARAM_NUMS_3, params))
    {
        printk("%s-->error waitResponse\r\n",__func__);
        return WL_FAILURE;
    }

    return WL_SUCCESS;
}

/*
 *
 */
static int8_t WiFiSpiDrv_getScannedData(uint8_t networkItem, char *ssid, int32_t *rssi, uint8_t *encr)
{
    tParam_t params[PARAM_NUMS_3] = {{WL_SSID_MAX_LENGTH, (char *)ssid}, {sizeof(*rssi), (char *)rssi}, {sizeof(*encr), (char *)encr}};

    // Send Command
    EspSpiDrv_sendCmd(GET_SCANNED_DATA_CMD, PARAM_NUMS_1);
    EspSpiDrv_sendParam(&networkItem, 1);
    EspSpiDrv_endCmd();

    ssid[0] = 0; // Default is empty SSID

    if (!EspSpiDrv_waitResponseParams(GET_SCANNED_DATA_CMD, PARAM_NUMS_3, params))
    {
        printk("%s-->error waitResponse\r\n",__func__);
        return WL_FAILURE;
    }

    ssid[params[0].paramLen] = '\0';

    return WL_SUCCESS;
}

/*
 *
 */
bool WiFiSpiDrv_getRemoteData(uint8_t sock, uint8_t *ip, uint16_t *port)
{
    tParam_t params[PARAM_NUMS_2] = {{4, (char *)ip}, {sizeof(*port), (char *)port}};

    // Send Command
    EspSpiDrv_sendCmd(GET_REMOTE_DATA_CMD, PARAM_NUMS_1);
    EspSpiDrv_sendParam(&sock, 1);
    EspSpiDrv_endCmd();

    if (!EspSpiDrv_waitResponseParams(GET_REMOTE_DATA_CMD, PARAM_NUMS_2, params))
    {
        printk("%s-->error waitResponse\r\n",__func__);
        return false;
    }

    return true;
}

// Public Methods

/*
 *
 */
void WiFiSpiDrv_wifiDriverInit(void)
{
    EspSpiProxy_begin();
}

/*
 *
 */
int8_t WiFiSpiDrv_wifiSetNetwork(const char *ssid, uint8_t ssid_len)
{
    // Test the input
    if (ssid_len > WL_SSID_MAX_LENGTH)
        return WL_FAILURE;

    // Send Command
    EspSpiDrv_sendCmd(SET_NET_CMD, PARAM_NUMS_1);
    EspSpiDrv_sendParam((uint8_t *)ssid, ssid_len);
    EspSpiDrv_endCmd();

    // Wait for reply
    int8_t _data = -1;
    uint8_t _dataLen = sizeof(_data);
    if (!EspSpiDrv_waitResponseCmd(SET_NET_CMD, PARAM_NUMS_1, &_data, &_dataLen))
    {
        printk("%s-->error waitResponse\r\n",__func__);
        _data = WL_FAILURE;
    }

    return _data;
}

/*
 * Connects to AP with given parameters
 * Returns: status - see getConnectionStatus()
 */
int8_t WiFiSpiDrv_wifiSetPassphrase(const char *ssid, const uint8_t ssid_len, const char *passphrase, const uint8_t len)
{
    // Test the input
    if (ssid_len > WL_SSID_MAX_LENGTH || len > WL_WPA_KEY_MAX_LENGTH)
        return WL_FAILURE;

    // Send Command
    EspSpiDrv_sendCmd(SET_PASSPHRASE_CMD, PARAM_NUMS_2);
    EspSpiDrv_sendParam((uint8_t *)(ssid), ssid_len);
    EspSpiDrv_sendParam((uint8_t *)(passphrase), len);
    EspSpiDrv_endCmd();

    // Wait for reply
    int8_t _data = -1;
    uint8_t _dataLen = sizeof(_data);
    if (!EspSpiDrv_waitResponseCmd(SET_PASSPHRASE_CMD, PARAM_NUMS_1, &_data, &_dataLen))
    {
        printk("%s-->error waitResponse\r\n",__func__);
        _data = WL_FAILURE;
    }

    return _data;
}

/*
 *
 */
bool WiFiSpiDrv_config(uint32_t local_ip, uint32_t gateway, uint32_t subnet, uint32_t dns_server1, uint32_t dns_server2)
{
    // Send Command
    EspSpiDrv_sendCmd(SET_IP_CONFIG_CMD, PARAM_NUMS_5);
    EspSpiDrv_sendParam((uint8_t *)(&local_ip), sizeof(local_ip));
    EspSpiDrv_sendParam((uint8_t *)(&gateway), sizeof(gateway));
    EspSpiDrv_sendParam((uint8_t *)(&subnet), sizeof(subnet));
    EspSpiDrv_sendParam((uint8_t *)(&dns_server1), sizeof(dns_server1));
    EspSpiDrv_sendParam((uint8_t *)(&dns_server2), sizeof(dns_server2));
    EspSpiDrv_endCmd();

    // Wait for reply
    uint8_t _data = false;
    uint8_t _dataLen = sizeof(_data);
    if (!EspSpiDrv_waitResponseCmd(SET_IP_CONFIG_CMD, PARAM_NUMS_1, &_data, &_dataLen))
    {
        printk("%s-->error waitResponse\r\n",__func__);
        _data = false;
    }

    return _data;
}

/*
 *
 */
uint8_t WiFiSpiDrv_disconnect()
{
    // Send Command
    EspSpiDrv_sendCmd(DISCONNECT_CMD, PARAM_NUMS_0);

    // Wait for reply
    uint8_t _data = -1;
    uint8_t _dataLen = sizeof(_data);
    if (!EspSpiDrv_waitResponseCmd(DISCONNECT_CMD, PARAM_NUMS_1, &_data, &_dataLen))
    {
        printk("%s-->error waitResponse\r\n",__func__);
        _data = WL_FAILURE;
    }

    return _data;
}

/*
 *
 */
uint8_t WiFiSpiDrv_getConnectionStatus()
{
    // Send Command
    EspSpiDrv_sendCmd(GET_CONN_STATUS_CMD, PARAM_NUMS_0);

    // Wait for reply
    uint8_t _data = -1;
    uint8_t _dataLen = sizeof(_data);
    if (!EspSpiDrv_waitResponseCmd(GET_CONN_STATUS_CMD, PARAM_NUMS_1, &_data, &_dataLen))
    {
        printk("%s-->error waitResponse\r\n",__func__);
    }

    return _data;
}

/*
 *
 */
uint8_t *WiFiSpiDrv_getMacAddress()
{
    // Send Command
    EspSpiDrv_sendCmd(GET_MACADDR_CMD, PARAM_NUMS_0);

    // Wait for reply
    uint8_t _dataLen = WL_MAC_ADDR_LENGTH;
    if (!EspSpiDrv_waitResponseCmd(GET_MACADDR_CMD, PARAM_NUMS_1, _mac, &_dataLen))
    {
        printk("%s-->error waitResponse\r\n",__func__);
    }

    if (_dataLen != WL_MAC_ADDR_LENGTH)
    {
        printk("error badReply\r\n");
    }

    return _mac;
}

/*
 *
 */
int8_t WiFiSpiDrv_getIpAddress(IPAddress_t *ip)
{
    int8_t status = WiFiSpiDrv_getNetworkData(_localIp, _subnetMask, _gatewayIp);
    if (status == WL_SUCCESS)
        for (uint8_t i = 0; i < WL_IPV4_LENGTH; i++)
            ip->address[i] = _localIp[i];

    return status;
}

/*
 *
 */
int8_t WiFiSpiDrv_getSubnetMask(IPAddress_t *mask)
{
    int8_t status = WiFiSpiDrv_getNetworkData(_localIp, _subnetMask, _gatewayIp);
    if (status == WL_SUCCESS)
        for (uint8_t i = 0; i < WL_IPV4_LENGTH; i++)
            mask->address[i] = _subnetMask[i];

    return status;
}

/*
 *
 */
int8_t WiFiSpiDrv_getGatewayIP(IPAddress_t *ip)
{
    int8_t status = WiFiSpiDrv_getNetworkData(_localIp, _subnetMask, _gatewayIp);
    if (status == WL_SUCCESS)
        for (uint8_t i = 0; i < WL_IPV4_LENGTH; i++)
            ip->address[i] = _gatewayIp[i];

    return status;
}

/*
 *
 */
char *WiFiSpiDrv_getCurrentSSID()
{
    // Send Command
    EspSpiDrv_sendCmd(GET_CURR_SSID_CMD, PARAM_NUMS_0);

    // Wait for reply
    uint8_t _dataLen = WL_SSID_MAX_LENGTH;
    if (!EspSpiDrv_waitResponseCmd(GET_CURR_SSID_CMD, PARAM_NUMS_1, (uint8_t *)(_ssid), &_dataLen))
    {
        printk("%s-->error waitResponse\r\n",__func__);
    }

    _ssid[_dataLen] = 0;  // terminate the string
    
    return _ssid;
}

/*
 *
 */
uint8_t *WiFiSpiDrv_getCurrentBSSID()
{
    // Send Command
    EspSpiDrv_sendCmd(GET_CURR_BSSID_CMD, PARAM_NUMS_0);

    // Wait for reply
    uint8_t _dataLen = WL_MAC_ADDR_LENGTH;
    if (!EspSpiDrv_waitResponseCmd(GET_CURR_BSSID_CMD, PARAM_NUMS_1, _bssid, &_dataLen))
    {
        printk("%s-->error waitResponse\r\n",__func__);
    }

    if (_dataLen != WL_MAC_ADDR_LENGTH)
    {
        printk("error badReply\r\n");
    }

    return _bssid;
}

/*
 *
 */
int32_t WiFiSpiDrv_getCurrentRSSI()
{
    // Send Command
    EspSpiDrv_sendCmd(GET_CURR_RSSI_CMD, PARAM_NUMS_0);

    // Wait for reply
    int32_t _rssi;
    uint8_t _dataLen = sizeof(_rssi);
    if (!EspSpiDrv_waitResponseCmd(GET_CURR_RSSI_CMD, PARAM_NUMS_1, (uint8_t *)(&_rssi), &_dataLen))
    {
        printk("%s-->error waitResponse\r\n",__func__);
        _dataLen = 0;
    }

    if (_dataLen != sizeof(_rssi))
    {
        printk("error badReply\r\n");
    }

    return _rssi;
}

/*
 *
 */
int8_t WiFiSpiDrv_startScanNetworks()
{
    // Send Command
    EspSpiDrv_sendCmd(START_SCAN_NETWORKS, PARAM_NUMS_0);

    int8_t _data = -1;
    uint8_t _dataLen = sizeof(_data);
    if (!EspSpiDrv_waitResponseCmd(START_SCAN_NETWORKS, PARAM_NUMS_1, (uint8_t *)(&_data), &_dataLen))
    {
        printk("%s-->error waitResponse\r\n",__func__);
    }

    return _data;
}

/*
 *
 */
int8_t WiFiSpiDrv_getScanNetworks()
{
    // Send Command
    EspSpiDrv_sendCmd(SCAN_NETWORKS, PARAM_NUMS_0);

    int8_t _data = -1;
    uint8_t _dataLen = sizeof(_data);
    if (!EspSpiDrv_waitResponseCmd(SCAN_NETWORKS, PARAM_NUMS_1, (uint8_t *)(&_data), &_dataLen))
    {
        printk("%s-->error waitResponse\r\n",__func__);
    }

    return _data;
}

/*
 *
 */
char *WiFiSpiDrv_getSSIDNetworks(uint8_t networkItem)
{
    int8_t status = WiFiSpiDrv_getScannedData(networkItem, _networkSsid, &_networkRssi, &_networkEncr);
    if (status != WL_SUCCESS)
        _networkSsid[0] = 0; // Empty string

    return _networkSsid;
}

/*
 *
 */
uint8_t WiFiSpiDrv_getEncTypeNetworks(uint8_t networkItem)
{
    int8_t status = WiFiSpiDrv_getScannedData(networkItem, _networkSsid, &_networkRssi, &_networkEncr);
    if (status != WL_SUCCESS)
        return 0;

    return _networkEncr;
}

/*
 *
 */
int32_t WiFiSpiDrv_getRSSINetworks(uint8_t networkItem)
{
    int8_t status = WiFiSpiDrv_getScannedData(networkItem, _networkSsid, &_networkRssi, &_networkEncr);
    if (status != WL_SUCCESS)
        return 0;

    return _networkRssi;
}

/*
    * Resolve the given hostname to an IP address.
    * param aHostname: Name to be resolved
    * param aResult: IPAddress structure to store the returned IP address
    * result: 1 if aIPAddrString was successfully converted to an IP address,
    *         0 error, hostname not found
    *        -1 error, command was not performed
    */
int8_t WiFiSpiDrv_getHostByName(const char *aHostname, IPAddress_t *aResult)
{
    // Send Command
    EspSpiDrv_sendCmd(GET_HOST_BY_NAME_CMD, PARAM_NUMS_1);
    EspSpiDrv_sendParam((uint8_t *)(aHostname), strlen(aHostname));
    EspSpiDrv_endCmd();

    // Wait for reply
    uint8_t _ipAddr[WL_IPV4_LENGTH];
    uint8_t _status;

    tParam_t params[PARAM_NUMS_2] = {{sizeof(_status), (char *)&_status}, {sizeof(_ipAddr), (char *)_ipAddr}};

    // Extended waiting time for status SPISLAVE_PREPARING_DATA
    for (int i = 1; i < 10; ++i)
    {
        if (EspSpiProxy_waitForSlaveTxReady() != SPISLAVE_TX_PREPARING_DATA)
            break;                            // The state is either SPISLAVE_TX_READY or SPISLAVE_TX_NODATA with timeout
        printk("Status: Preparing data\r\n"); ///
    }

    if (!EspSpiDrv_waitResponseParams(GET_HOST_BY_NAME_CMD, PARAM_NUMS_2, params))
    {
        printk("%s-->error waitResponse\r\n",__func__);
        return WL_FAILURE;
    }

    if (params[0].paramLen != sizeof(_status) || params[1].paramLen != sizeof(_ipAddr))
    {
        printk("error badReply\r\n");
        return 0;
    }

    for (uint8_t i = 0; i < WL_IPV4_LENGTH; i++)
        aResult->address[i] = _ipAddr[i];

    // aResult = _ipAddr;

    return _status;
}

/*
 *
 */
char *WiFiSpiDrv_getFwVersion()
{
    // Send Command
    EspSpiDrv_sendCmd(GET_FW_VERSION_CMD, PARAM_NUMS_0);

    // Wait for reply
    uint8_t _dataLen = WL_FW_VER_LENGTH;
    if (!EspSpiDrv_waitResponseCmd(GET_FW_VERSION_CMD, PARAM_NUMS_1, (uint8_t *)fwVersion, &_dataLen))
    {
        printk("%s-->error waitResponse\r\n",__func__);
    }

    return fwVersion;
}

/*
 * Perform remote software reset of the ESP8266 module.
 * The reset succeedes only if the SPI communication is not broken.
 * The function does not wait for the ESP8266.
 */
void WiFiSpiDrv_softReset(void)
{
    // Send Command
    EspSpiDrv_sendCmd(SOFTWARE_RESET_CMD, PARAM_NUMS_0);

    // Wait for reply
    uint8_t _dataLen = WL_FW_VER_LENGTH;
    if (!EspSpiDrv_waitResponseCmd(SOFTWARE_RESET_CMD, PARAM_NUMS_0, NULL, NULL))
    {
        printk("%s-->error waitResponse\r\n",__func__);
    }
}

/*
 *
 */
char *WiFiSpiDrv_getProtocolVersion()
{
    // Send Command
    EspSpiDrv_sendCmd(GET_PROTOCOL_VERSION_CMD, PARAM_NUMS_0);

    // Wait for reply
    uint8_t _dataLen = WL_PROTOCOL_VER_LENGTH;
    if (!EspSpiDrv_waitResponseCmd(GET_PROTOCOL_VERSION_CMD, PARAM_NUMS_1, (uint8_t *)protocolVersion, &_dataLen))
    {
        printk("%s-->error waitResponse\r\n",__func__);
    }

    return protocolVersion;
}

uint32_t IPAddress_to_uint32(IPAddress_t *ip)
{
    return (uint32_t)(ip->address[3] << 24 | ip->address[2] << 16 | ip->address[1] << 8 | ip->address[0]);
}

IPAddress_t uint8_to_IPAddress(uint8_t *ip)
{
    IPAddress_t ret;
    for (uint8_t i = 0; i < WL_IPV4_LENGTH; i++)
        ret.address[i] = ip[i];
    return ret;
}

char *IPAddress_to_string(IPAddress_t *ip)
{
    char buf[32];
    sprintf(buf, "%d:%d:%d:%d", ip->address[3], ip->address[2], ip->address[1], ip->address[0]);
    return buf;
}