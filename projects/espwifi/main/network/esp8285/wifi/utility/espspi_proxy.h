/*
  espspi_proxy.h - Library for Arduino SPI connection with ESP8266
  
  Copyright (c) 2017 Jiri Bilek. All right reserved.

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

  -------------

    WiFi SPI Safe Master for connecting with ESP8266
    On ESP8266 must be flashed WiFiSPIESP application
    Connect the SPI Master device to the following pins on the esp8266:

            ESP8266         |
    GPIO    NodeMCU   Name  |   Uno
   ===================================
     15       D8       SS   |   D10**
     13       D7      MOSI  |   D11
     12       D6      MISO  |   D12
     14       D5      SCK   |   D13

    **) User changeable

    Based on Hristo Gochkov's SPISlave library.
*/

#ifndef _ESPSPI_PROXY_H
#define _ESPSPI_PROXY_H

/* clang-format off */

// The command codes are fixed by ESP8266 hardware
#define CMD_WRITESTATUS             0x01
#define CMD_WRITEDATA               0x02
#define CMD_READDATA                0x03
#define CMD_READSTATUS              0x04

// Message indicators
#define MESSAGE_FINISHED            0xDF
#define MESSAGE_CONTINUES           0xDC

// SPI Status
enum
{
    SPISLAVE_RX_BUSY,
    SPISLAVE_RX_READY,
    SPISLAVE_RX_CRC_PROCESSING,
    SPISLAVE_RX_ERROR
};

enum
{
    SPISLAVE_TX_NODATA,
    SPISLAVE_TX_READY,
    SPISLAVE_TX_PREPARING_DATA,
    SPISLAVE_TX_WAITING_FOR_CONFIRM
};

// How long we will wait for slave to be ready
#define SLAVE_RX_READY_TIMEOUT 3000UL
#define SLAVE_TX_READY_TIMEOUT 3000UL

// How long will be SS held high when starting transmission
// #define SS_PULSE_DELAY_MICROSECONDS 50

/* clang-format on */

void EspSpiProxy_begin(void);
void EspSpiProxy_pulseSS(uint8_t start);
uint16_t EspSpiProxy_readStatus();
void EspSpiProxy_writeStatus(uint8_t status);
void EspSpiProxy_readData(uint8_t *buf);
// void EspSpiProxy_writeData(uint8_t *data, size_t len);
void EspSpiProxy_writeData(uint8_t *data);
void EspSpiProxy_flush(uint8_t indicator);
void EspSpiProxy_writeByte(uint8_t b);
uint8_t EspSpiProxy_readByte();
int8_t EspSpiProxy_waitForSlaveRxReady();
int8_t EspSpiProxy_waitForSlaveTxReady();
void EspSpiProxy_hardReset(int8_t hwResetPin);
int8_t EspSpiProxy_waitForSlaveRxConfirmation();
#endif
