/*
  espspi_drv.cpp - Library for Arduino SPI connection with ESP8266
  
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
#include "espspi_drv.h"

#define _MIN    1

#if _MIN
// Read and check one byte from the input
#define READ_AND_CHECK_BYTE(c)    \
    if (EspSpiProxy_readByte() != (c)) \
        return 0;
#else
// Read and check one byte from the input
#define READ_AND_CHECK_BYTE(c, err)    \
    if (EspSpiProxy_readByte() != (c)){} \
        printk("err:%s\r\n",err);
        return 0;
        }
#endif

/* 
    Sends a command to ESP. If numParam == 0 ends the command otherwise keeps it open.
    
    Cmd Struct Message
   _______________________________________________________________________
  | START CMD | C/R  | CMD  | N.PARAM | PARAM LEN | PARAM  | .. | END CMD |
  |___________|______|______|_________|___________|________|____|_________|
  |   8 bit   | 1bit | 7bit |  8bit   |   8bit    | nbytes | .. |   8bit  |
  |___________|______|______|_________|___________|________|____|_________|

  The last byte (position 31) is crc8.
*/
void EspSpiDrv_sendCmd(const uint8_t cmd, const uint8_t numParam)
{
    WAIT_FOR_SLAVE_RX_READY();

    // Send Spi START CMD
    EspSpiProxy_writeByte(START_CMD);

    // Send Spi C + cmd
    EspSpiProxy_writeByte(cmd & ~(REPLY_FLAG));

    // Send Spi numParam
    EspSpiProxy_writeByte(numParam);

    // If numParam == 0 send END CMD and flush
    if (numParam == 0)
        EspSpiDrv_endCmd();
}

/*
   Ends a command and flushes the buffer
 */
void EspSpiDrv_endCmd()
{
    EspSpiProxy_writeByte(END_CMD);
    EspSpiProxy_flush(MESSAGE_FINISHED);
}

/*
    Sends a parameter.
    param ... parameter value
    param_len ... length of the parameter
 */
void EspSpiDrv_sendParam(const uint8_t *param, const uint8_t param_len)
{
    // Send paramLen
    EspSpiProxy_writeByte(param_len);

    // Send param data
    for (int i = 0; i < param_len; ++i)
        EspSpiProxy_writeByte(param[i]);
}

// /*
//     Sends a 8 bit integer parameter. Sends high byte first.
//     param ... parameter value
//  */
// void EspSpiDrv_sendParam(const uint8_t param)
// {
//     // Send paramLen
//     EspSpiProxy_writeByte(1);

//     // Send param data
//     EspSpiProxy_writeByte(param);
// }

/*
    Sends a buffer as a parameter.
    Parameter length is 16 bit.
 */
void EspSpiDrv_sendBuffer(const uint8_t *param, uint16_t param_len)
{
    // Send paramLen
    EspSpiProxy_writeByte(param_len & 0xff);
    EspSpiProxy_writeByte(param_len >> 8);

    // Send param data
    for (uint16_t i = 0; i < param_len; ++i)
        EspSpiProxy_writeByte(param[i]);
}

/*
    Gets a response from the ESP
    cmd ... command id
    numParam ... number of parameters - currently supported 0 or 1
    param  ... pointer to space for the first parameter
    param_len ... max length of the first parameter, returns actual length
 */
uint8_t EspSpiDrv_waitResponseCmd(const uint8_t cmd, uint8_t numParam, uint8_t *param, uint8_t *param_len)
{
    WAIT_FOR_SLAVE_TX_READY();

#if !_MIN
    READ_AND_CHECK_BYTE(START_CMD, "Start");
    READ_AND_CHECK_BYTE(cmd | REPLY_FLAG, "Cmd");
    READ_AND_CHECK_BYTE(numParam, "Param");
#else
    READ_AND_CHECK_BYTE(START_CMD);
    READ_AND_CHECK_BYTE(cmd | REPLY_FLAG);
    READ_AND_CHECK_BYTE(numParam);
#endif

    if (numParam == 1)
    {
        // Reads the length of the first parameter
        uint8_t len = EspSpiProxy_readByte();

        // Reads the parameter, checks buffer overrun
        for (uint8_t ii = 0; ii < len; ++ii)
        {
            if (ii < *param_len)
                param[ii] = EspSpiProxy_readByte();
        }

        // Sets the actual length of the parameter
        if (len < *param_len)
            *param_len = len;
    }
    else if (numParam != 0)
        return 0; // Bad number of parameters

#if !_MIN
    READ_AND_CHECK_BYTE(END_CMD, "End");
#else
    READ_AND_CHECK_BYTE(END_CMD);
#endif

    return 1;
}

/*
    Gets a response from the ESP
    cmd ... command id
    numParam ... number of parameters - currently supported 0 or 1
    param  ... pointer to space for the first parameter
    param_len ... max length of the first parameter (16 bit integer), returns actual length
 */
uint8_t EspSpiDrv_waitResponseCmd16(const uint8_t cmd, uint8_t numParam, uint8_t *param, uint16_t *param_len)
{
    WAIT_FOR_SLAVE_TX_READY();
    
#if !_MIN
    READ_AND_CHECK_BYTE(START_CMD, "Start");
    READ_AND_CHECK_BYTE(cmd | REPLY_FLAG, "Cmd");
    READ_AND_CHECK_BYTE(numParam, "Param");
#else
    READ_AND_CHECK_BYTE(START_CMD);
    READ_AND_CHECK_BYTE(cmd | REPLY_FLAG);
    READ_AND_CHECK_BYTE(numParam);
#endif

    if (numParam == 1)
    {
        // Reads the length of the first parameter
        uint16_t len = EspSpiProxy_readByte() << 8;
        len |= EspSpiProxy_readByte();
        // Reads the parameter, checks buffer overrun
        for (uint16_t ii = 0; ii < len; ++ii)
        {
            if (ii < *param_len)
                param[ii] = EspSpiProxy_readByte();
        }

        // Sets the actual length of the parameter
        if (len < *param_len)
            *param_len = len;
    }
    else if (numParam != 0)
        return 0; // Bad number of parameters

#if !_MIN
    READ_AND_CHECK_BYTE(END_CMD, "End");
#else
    READ_AND_CHECK_BYTE(END_CMD);
#endif

    return 1;
}

/*
    Reads a response from the ESP. Decodes parameters and puts them into a return structure
 */
int8_t EspSpiDrv_waitResponseParams(const uint8_t cmd, uint8_t numParam, tParam_t *params)
{
    WAIT_FOR_SLAVE_TX_READY();

#if !_MIN
    READ_AND_CHECK_BYTE(START_CMD, "Start");
    READ_AND_CHECK_BYTE(cmd | REPLY_FLAG, "Cmd");
    READ_AND_CHECK_BYTE(numParam, "Param");
#else
    READ_AND_CHECK_BYTE(START_CMD);
    READ_AND_CHECK_BYTE(cmd | REPLY_FLAG);
    READ_AND_CHECK_BYTE(numParam);
#endif

    if (numParam > 0)
    {
        for (uint8_t i = 0; i < numParam; ++i)
        {
            // Reads the length of the first parameter
            uint8_t len = EspSpiProxy_readByte();

            // Reads the parameter, checks buffer overrun
            for (uint8_t ii = 0; ii < len; ++ii)
            {
                if (ii < params[i].paramLen)
                    params[i].param[ii] = EspSpiProxy_readByte();
            }

            // Sets the actual length of the parameter
            if (len < params[i].paramLen)
                params[i].paramLen = len;
        }
    }

#if !_MIN
    READ_AND_CHECK_BYTE(END_CMD, "End");
#else
    READ_AND_CHECK_BYTE(END_CMD);
#endif

    return 1;
}
