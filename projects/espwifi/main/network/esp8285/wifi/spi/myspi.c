#include <stdlib.h>
#include "gpiohs.h"
#include "myspi.h"
#include "sleep.h"
#include "sysctl.h"

#include <utils.h>
#include "spi.h"
#include "printf.h"

#include "global_config.h"


static spi_transfer_width_t sipeed_spi_get_frame_size(size_t data_bit_length);
static void sipeed_spi_set_tmod(uint8_t spi_num, uint32_t tmod);
static void sipeed_spi_transfer_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select,
                                              const uint8_t *tx_buff, uint8_t *rx_buff, size_t len);

/* SPI端口初始化 */
void my_spi_init(void)
{
    printk("hard spi\r\n");
    //cs
    gpiohs_set_drive_mode(CONFIG_PIN_NUM_WIFI_SPI_CS, GPIO_DM_OUTPUT);
    gpiohs_set_pin(CONFIG_PIN_NUM_WIFI_SPI_CS, 1);
    //init SPI_DEVICE_1
    spi_init(SPI_DEVICE_1, SPI_WORK_MODE_1, SPI_FF_STANDARD, 8, 0);
    printk("set spi clk:%d\r\n", spi_set_clk_rate(SPI_DEVICE_1, 1000000 * 8)); /*set clk rate*/
}

void my_spi_cs_set(void)
{
    gpiohs_set_pin(CONFIG_PIN_NUM_WIFI_SPI_CS, GPIO_PV_HIGH);
}

void my_spi_cs_clr(void)
{
    gpiohs_set_pin(CONFIG_PIN_NUM_WIFI_SPI_CS, GPIO_PV_LOW);
}

uint8_t my_spi_rw(uint8_t data)
{
    uint8_t c;
    sipeed_spi_transfer_data_standard(SPI_DEVICE_1, SPI_CHIP_SELECT_0, &data, &c, 1);
    return c;
}

void my_spi_rw_len(uint8_t *send, uint8_t *recv, uint32_t len)
{
    if(send == NULL && recv == NULL)
    {
        printk(" buffer is null\r\n");
        return;
    }

#if 0
    spi_init(SPI_DEVICE_1, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
#endif

    //only send
    if(send && recv == NULL)
    {
        spi_send_data_standard(SPI_DEVICE_1, SPI_CHIP_SELECT_0, NULL, 0, send, len);
        return;
    }

    //only recv
    if(send == NULL && recv)
    {
        spi_receive_data_standard(SPI_DEVICE_1, SPI_CHIP_SELECT_0, NULL, 0, recv, len);
        return;
    }

    //send and recv
    if(send && recv)
    {
        sipeed_spi_transfer_data_standard(SPI_DEVICE_1, SPI_CHIP_SELECT_0, send, recv, len);
        return;
    }
    return;
}

uint64_t get_millis(void)
{
    return sysctl_get_time_us() / 1000;
}

///////////////////////////////////////////////////////////////////////////////
static spi_transfer_width_t sipeed_spi_get_frame_size(size_t data_bit_length)
{
    if(data_bit_length < 8)
        return SPI_TRANS_CHAR;
    else if(data_bit_length < 16)
        return SPI_TRANS_SHORT;
    return SPI_TRANS_INT;
}

static void sipeed_spi_set_tmod(uint8_t spi_num, uint32_t tmod)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);
    volatile spi_t *spi_handle = spi[spi_num];
    uint8_t tmod_offset = 0;
    switch(spi_num)
    {
        case 0:
        case 1:
            tmod_offset = 8;
            break;
        case 2:
            configASSERT(!"Spi Bus 2 Not Support!");
            break;
        case 3:
        default:
            tmod_offset = 10;
            break;
    }
    set_bit(&spi_handle->ctrlr0, 3 << tmod_offset, tmod << tmod_offset);
}

static void sipeed_spi_transfer_data_standard(spi_device_num_t spi_num, spi_chip_select_t chip_select,
                                              const uint8_t *tx_buff, uint8_t *rx_buff, size_t len)
{
    configASSERT(spi_num < SPI_DEVICE_MAX && spi_num != 2);
    configASSERT(len > 0);
    size_t index, fifo_len;
    size_t rx_len = len;
    size_t tx_len = rx_len;
    sipeed_spi_set_tmod(spi_num, SPI_TMOD_TRANS_RECV);

    volatile spi_t *spi_handle = spi[spi_num];

    uint8_t dfs_offset;
    switch(spi_num)
    {
        case 0:
        case 1:
            dfs_offset = 16;
            break;
        case 2:
            configASSERT(!"Spi Bus 2 Not Support!");
            break;
        case 3:
        default:
            dfs_offset = 0;
            break;
    }
    uint32_t data_bit_length = (spi_handle->ctrlr0 >> dfs_offset) & 0x1F;
    spi_transfer_width_t frame_width = sipeed_spi_get_frame_size(data_bit_length);
    spi_handle->ctrlr1 = (uint32_t)(tx_len / frame_width - 1);
    spi_handle->ssienr = 0x01;
    spi_handle->ser = 1U << chip_select;
    uint32_t i = 0;
    while(tx_len)
    {
        fifo_len = 32 - spi_handle->txflr;
        fifo_len = fifo_len < tx_len ? fifo_len : tx_len;
        switch(frame_width)
        {
            case SPI_TRANS_INT:
                fifo_len = fifo_len / 4 * 4;
                for(index = 0; index < fifo_len / 4; index++)
                    spi_handle->dr[0] = ((uint32_t *)tx_buff)[i++];
                break;
            case SPI_TRANS_SHORT:
                fifo_len = fifo_len / 2 * 2;
                for(index = 0; index < fifo_len / 2; index++)
                    spi_handle->dr[0] = ((uint16_t *)tx_buff)[i++];
                break;
            default:
                for(index = 0; index < fifo_len; index++)
                    spi_handle->dr[0] = tx_buff[i++];
                break;
        }
        tx_len -= fifo_len;
    }

    while((spi_handle->sr & 0x05) != 0x04)
        ;
    i = 0;
    while(rx_len)
    {
        fifo_len = spi_handle->rxflr;
        fifo_len = fifo_len < rx_len ? fifo_len : rx_len;
        switch(frame_width)
        {
            case SPI_TRANS_INT:
                fifo_len = fifo_len / 4 * 4;
                for(index = 0; index < fifo_len / 4; index++)
                    ((uint32_t *)rx_buff)[i++] = spi_handle->dr[0];
                break;
            case SPI_TRANS_SHORT:
                fifo_len = fifo_len / 2 * 2;
                for(index = 0; index < fifo_len / 2; index++)
                    ((uint16_t *)rx_buff)[i++] = (uint16_t)spi_handle->dr[0];
                break;
            default:
                for(index = 0; index < fifo_len; index++)
                    rx_buff[i++] = (uint8_t)spi_handle->dr[0];
                break;
        }

        rx_len -= fifo_len;
    }
    spi_handle->ser = 0x00;
    spi_handle->ssienr = 0x00;
}
