/*
  espspi_proxy.cpp - Library for Arduino SPI connection with ESP8266
  
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
#include <stdio.h>
#include <stdbool.h>
#include "espspi_proxy.h"
#include "config.h"

#include "printf.h"

#include "myspi.h"
#include "sleep.h"
#include "gpiohs.h"

static uint8_t buffer[32];
static uint8_t buflen;
static uint8_t bufpos;
static uint8_t zero_buf[32];

static uint8_t crc8(uint8_t *buffer, uint8_t bufLen);

void EspSpiProxy_pulseSS(uint8_t start)
{
  if (start)
  { // tested ok: 5, 15 / 5
    // digitalWrite(_ss_pin, HIGH);
    my_spi_cs_set();
    usleep(3);
    // digitalWrite(_ss_pin, LOW);
    my_spi_cs_clr();
    usleep(25); // 10us is low (some errors), 20 us is safe (no errors)
  }
  else
  {
    // digitalWrite(_ss_pin, HIGH);
    my_spi_cs_set();
    usleep(3);
    // digitalWrite(_ss_pin, LOW);
    my_spi_cs_clr();
  }
}

void EspSpiProxy_begin(void)
{
  buflen = 0;
  my_spi_cs_clr();

  for (uint8_t i = 0; i < 32; i++)
  {
    zero_buf[i] = 0x0;
  }
}

uint16_t EspSpiProxy_readStatus()
{
  EspSpiProxy_pulseSS(true);

  // my_spi_rw(CMD_READSTATUS);
  // uint32_t status = (my_spi_rw(0) | ((uint32_t)(my_spi_rw(0)) << 8) | ((uint32_t)(my_spi_rw(0)) << 16) | ((uint32_t)(my_spi_rw(0)) << 24));

  my_spi_rw(CMD_READSTATUS);

  // uint32_t status = (my_spi_rw(0) | ((uint32_t)(my_spi_rw(0)) << 8) | ((uint32_t)(my_spi_rw(0)) << 16) | ((uint32_t)(my_spi_rw(0)) << 24));

  uint16_t status = my_spi_rw(0) | ((uint16_t)(my_spi_rw(0) << 8));

  EspSpiProxy_pulseSS(false);

  return status;
}

void EspSpiProxy_writeStatus(uint8_t status)
{
  EspSpiProxy_pulseSS(true);

  my_spi_rw(CMD_WRITESTATUS);
  my_spi_rw(status);
  my_spi_rw(status ^ 0xff); // byte inverted value as a check

  EspSpiProxy_pulseSS(false);
}

void EspSpiProxy_readData(uint8_t *buf)
{
  EspSpiProxy_pulseSS(true);

  my_spi_rw(CMD_READDATA);
  my_spi_rw(0x00);

#if 0
  for (uint8_t i = 0; i < 32; i++)
  {
    buf[i] = my_spi_rw(0); // the value is not important
  }
#else
  my_spi_rw_len(zero_buf, buf, 32);
#endif

  EspSpiProxy_pulseSS(false);
}

// void EspSpiProxy_writeData(uint8_t *data, size_t len)
void EspSpiProxy_writeData(uint8_t *data)
{
  EspSpiProxy_pulseSS(true);

  my_spi_rw(CMD_WRITEDATA);
  my_spi_rw(0x00);
#if 0
  uint8_t i = 0;
  while (len-- && i < 32)
  {
    my_spi_rw(data[i++]);
  }
  while (i++ < 32)
  {
    my_spi_rw(0);
  }
#else
  for (uint8_t i = 0; i < 32; ++i)
    my_spi_rw(data[i]);
#endif
  EspSpiProxy_pulseSS(false);
}

void EspSpiProxy_flush(uint8_t indicator)
{
  // Is buffer empty?
  if (buflen == 0)
    return;

  // Message state indicator
  buffer[0] = indicator;

#if 0
  // Wait for slave ready
  // TODO: move the waiting loop to writeByte (defer waiting)
  uint8_t s = waitForSlaveRxReady();
  if (s == SPISLAVE_RX_READY || s == SPISLAVE_RX_ERROR) // Error state can't be recovered, we can send new data
  {
    // Send the buffer
    EspSpiProxy_writeData(buffer, buflen + 1);
  }
#else
  // Pad the buffer with zeroes
  while (buflen < 30)
    buffer[++buflen] = 0;

  // Compute CRC8
  buffer[31] = crc8(buffer, 31);

  // Wait for slave ready
  // TODO: move the waiting loop to writeByte (defer waiting)
  uint8_t s = EspSpiProxy_waitForSlaveRxReady();
  if (s == SPISLAVE_RX_READY || s == SPISLAVE_RX_ERROR) // Error state can't be recovered, we can send new data
  {
    // Try to send the buffer 10 times, we may not be stuck in an endless loop
    for (uint8_t i = 0; i < 10; ++i)
    {
      // Send the buffer
      EspSpiProxy_writeData(buffer);

      if (EspSpiProxy_waitForSlaveRxConfirmation())
        break;
      else
        printk("Bad CRC, retransmitting\r\n");
    }
  }

#endif
  buflen = 0;
}

void EspSpiProxy_writeByte(uint8_t b)
{
  bufpos = 0; // discard input data in the buffer

  if (buflen >= 30)
    EspSpiProxy_flush(MESSAGE_CONTINUES);

  buffer[++buflen] = b;
}

uint8_t EspSpiProxy_readByte()
{
  buflen = 0; // discard output data in the buffer

  if (bufpos >= 31) // the buffer segment was read
  {
    if (buffer[0] != MESSAGE_CONTINUES)
      return 0;

    bufpos = 0; // read next chunk

    // Wait for the slave
    EspSpiProxy_waitForSlaveTxReady();
  }

  if (bufpos == 0) // buffer empty
  {
    uint64_t thisTime = get_millis() + 1000;

    do
    {
      EspSpiProxy_readData(buffer);

      if (buffer[31] == crc8(buffer, 31))
        break;

      printk("Bad CRC, request repeated\r\n");

    } while (get_millis() - thisTime < 200);

    if (buffer[0] != MESSAGE_FINISHED && buffer[0] != MESSAGE_CONTINUES)
      return 0; // incorrect message (should not happen)

    bufpos = 1;
  }
  return buffer[bufpos++];
}

/*
  Waits for slave receiver ready status.
  Return: status (SPISLAVE_RX_BUSY, SPISPLAVE_RX_READY or SPISLAVE_RX_PREPARING_DATA)
  */
int8_t EspSpiProxy_waitForSlaveRxReady()
{
  uint64_t startTime = get_millis();
  uint16_t status = 0;
#if 0
  do
  {
    status = EspSpiProxy_readStatus();

    if (((status >> 28) == SPISLAVE_RX_READY))
      return (status >> 28); // status

    usleep(10);
    // yield();
  } while ((get_millis() & 0x0fffffff) < endTime);

  printk("Slave rx is not ready\r\n");
  printk("Returning: %x\r\n", status >> 28);

  return (status >> 28); // timeout
#else
  do
  {
    status = EspSpiProxy_readStatus();

    if ((status & 0xff) == ((status >> 8) ^ 0xff)) // check the xor
    {
      uint16_t s = (status >> 4) & 0x0f;
      if (s == SPISLAVE_RX_READY || s == SPISLAVE_RX_ERROR) // From the perspective of rx state the error is the same as ready
        return ((status >> 4) & 0x0f);                      // status
    }

    // yield();
  } while (get_millis() - startTime < SLAVE_RX_READY_TIMEOUT);

  printk("Slave rx is not ready, status %02x\r\n", (status >> 4) & 0x0f);

  return ((status >> 4) & 0x0f); // timeout

#endif
}

/*
  Waits for slave transmitter ready status.
  Return: status (SPISLAVE_TX_NODATA, SPISPLAVE_TX_READY
  */
int8_t EspSpiProxy_waitForSlaveTxReady()
{
#if 0
  uint64_t endTime = (get_millis() & 0x0fffffff) + SLAVE_RX_READY_TIMEOUT;
  uint32_t status = SPISLAVE_TX_NODATA;

  do
  {
    status = EspSpiProxy_readStatus();

    if ((((status >> 24) & 0x0f) == SPISLAVE_TX_READY))
      return ((status >> 24) & 0x0f); // status

    usleep(10);
    // yield();
  } while ((get_millis() & 0x0fffffff) < endTime);

  printk("Slave tx is not ready\r\n");
  printk("Returning: %x\r\n", (status >> 24) & 0x0f);

  return ((status >> 24) & 0x0f); // timeout
#else
  uint64_t startTime = get_millis();
  uint16_t status = 0;
  uint16_t err_cnt = 0;

  do
  {
    status = EspSpiProxy_readStatus();

    if ((status & 0xff) == ((status >> 8) ^ 0xff)) // check the xor
    {
      if ((status & 0x0f) == SPISLAVE_TX_READY)
        return (status & 0x0f); // status    
      else if ((status & 0x0f) == SPISLAVE_TX_NODATA)
      {
        msleep(1);
        err_cnt++;
        if (err_cnt > 50)
        {
          printk("return err\r\n");
          return 0xff;
        }
      }
      else
      {
        err_cnt = 0;
      }
    }
    else
    {
      //no reach
      msleep(1);
      err_cnt++;
      if (err_cnt > 50)
      {
        printk("return err\r\n");
        return 0xff;
      }
    }

    // if(get_millis() - startTime > 1000)
    // {
    //   printk("waitForSlaveTxReady > 1000ms, stat:%02x\t %02x \t%02x\r\n",status,(status & 0xff),((status >> 8) ^ 0xff));
    //   if ((status & 0x0f) == SPISLAVE_TX_NODATA)
    //   {
    //     // Send Command
    //     // EspSpiDrv_sendCmd(GET_PROTOCOL_VERSION_CMD, PARAM_NUMS_0);
    //     // msleep(100);
    //     return 0xff;
    //   }
    // }

    // yield();
  } while (get_millis() - startTime < SLAVE_TX_READY_TIMEOUT);

  printk("Slave tx is not ready, status %02x\r\n", status & 0x0f);

  return (status & 0x0f); // timeout

#endif
}

/*
  Waits for slave receiver read confirmation or rejection status.
  SPISLAVE_RX_CRC_PROCESSING holds the loop, SPISLAVE_RX_ERROR exits with error, other exits ok
  Return: receiver read data ok
*/
int8_t EspSpiProxy_waitForSlaveRxConfirmation()
{
  uint32_t startTime = get_millis();
  uint16_t status = 0;

  do
  {
    status = EspSpiProxy_readStatus();

    if ((status & 0xff) == ((status >> 8) ^ 0xff)) // check the xor
    {
      uint8_t stat = (status >> 4) & 0x0f;

      if (stat == SPISLAVE_RX_ERROR)
        return 0; // Error status
      else if (stat != SPISLAVE_RX_CRC_PROCESSING)
        return 1; // OK statu
      // Slave is still processing the CRC
    }
    // yield();
  } while (get_millis() - startTime < SLAVE_RX_READY_TIMEOUT);

  printk("Slave rx (confirm) is not ready, status %02x\r\n", (status >> 4) & 0x0f);

  return 1; // timeout - we can't retransmit in order not to replay old data, assume everything is ok
}
/*
* Compute crc8 with polynom 0x107 (x^8 + x^2 + x + 1)
* Lookup table idea: https://lentz.com.au/blog/tag/crc-table-generator
*/
static uint8_t crc8(uint8_t *buffer, uint8_t bufLen)
{
  // const static uint8_t POLY = 0x07;

  static const uint8_t tableLow[] = {0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
                                     0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D};
  static const uint8_t tableHigh[] = {0x00, 0x70, 0xE0, 0x90, 0xC7, 0xB7, 0x27, 0x57,
                                      0x89, 0xF9, 0x69, 0x19, 0x4E, 0x3E, 0xAE, 0xDE};

  uint8_t crcValue = 0x00;

  for (int i = 0; i < bufLen; ++i)
  {
    crcValue ^= buffer[i];
    crcValue = tableLow[(crcValue & 0x0f)] ^ tableHigh[((crcValue >> 4) & 0x0f)];
  }

  return crcValue;
}

/*
* Puts the SS low (required for successfull boot) and resets the ESP
*/
void EspSpiProxy_hardReset(int8_t hwResetPin)
{
  my_spi_cs_clr(); // Safe value for ESP8266 GPIO15
  gpiohs_set_pin(hwResetPin, 0);
  msleep(50);
  gpiohs_set_pin(hwResetPin, 1);
  msleep(200);
}
