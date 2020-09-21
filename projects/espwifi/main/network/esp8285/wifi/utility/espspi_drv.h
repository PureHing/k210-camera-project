/*
  espspi_drv.h - Library for Arduino SPI connection with ESP8266
  
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
*/

#ifndef _ESPSPI_DRV_H_INCLUDED
#define _ESPSPI_DRV_H_INCLUDED

#include "wifi_spi.h"
#include "wifispi_drv.h"
#include "espspi_proxy.h"

#define WAIT_FOR_SLAVE_RX_READY() \
  EspSpiProxy_waitForSlaveRxReady();

#define WAIT_FOR_SLAVE_TX_READY() \
  EspSpiProxy_waitForSlaveTxReady();

void EspSpiDrv_sendCmd(const uint8_t cmd, const uint8_t numParam);
void EspSpiDrv_endCmd();
void EspSpiDrv_sendParam(const uint8_t *param, const uint8_t param_len);
// void EspSpiDrv_sendParam(const uint8_t param);
void EspSpiDrv_sendBuffer(const uint8_t *param, uint16_t param_len);
uint8_t EspSpiDrv_waitResponseCmd(const uint8_t cmd, uint8_t numParam, uint8_t *param, uint8_t *param_len);
uint8_t EspSpiDrv_waitResponseCmd16(const uint8_t cmd, uint8_t numParam, uint8_t *param, uint16_t *param_len);
int8_t EspSpiDrv_waitResponseParams(const uint8_t cmd, uint8_t numParam, tParam_t *params);

#endif
