#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sleep.h"
#include "sysctl.h"

#include "WiFiSpiClient.h"

#include "http_save_file.h"

/* clang-format off */
#define BUFFSIZE                1024
/* clang-format on */

static uint32_t l_recv_len = 0;

static uint32_t l_recv_file_buf_len = 0;
static uint8_t *l_recv_file_buf = NULL;

static uint32_t l_recv_header_buf_len = 0;
static uint8_t *l_recv_header_buf = NULL;

static unsigned char l_tmp_buf[BUFFSIZE + 1] = {0};

/*read buffer by byte still delim ,return read bytes counts*/
static uint32_t read_until(char *buffer, char delim, uint32_t len)
{
    //  /*TODO: delim check,buffer check,further: do an buffer length limited*/
    uint32_t i = 0;
    while(buffer[i] != delim && i < len)
    {
        ++i;
    }
    return i + 1;
}

/* resolve a packet from http socket
 * return 1 if packet including \r\n\r\n that means http packet header finished,start to receive packet body
 * otherwise return 0
 * */
static uint32_t read_past_header(uint8_t text[], uint32_t total_len)
{
    /* i means current position */
    uint32_t i = 0, i_read_len = 0;

    while(text[i] != 0 && i < total_len)
    {
        i_read_len = read_until(&text[i], '\n', total_len);
        // if we resolve \r\n line,we think packet header is finished
        if(i_read_len == 2)
        {
            uint32_t i_write_len = total_len - (i + 2);

            l_recv_len = 0;
            memset(l_recv_file_buf, 0, l_recv_file_buf_len);
            memcpy(l_recv_file_buf, &(text[i + 2]), i_write_len);
            l_recv_len = i_write_len;

            memset(l_recv_header_buf, 0, l_recv_header_buf_len);
            memcpy(l_recv_header_buf, text, (i > l_recv_header_buf_len) ? l_recv_header_buf_len : i);
            l_recv_header_buf[l_recv_header_buf_len] = 0;

            return 1;
        }
        i += i_read_len;
    }
    return 0;
}

uint32_t http_save_file(uint8_t sock, char *resp_header, uint32_t resp_header_len, uint8_t *file, uint32_t file_len)
{
    //check file buf
    l_recv_file_buf = file;
    l_recv_file_buf_len = file_len;
    l_recv_header_buf = resp_header;
    l_recv_header_buf_len = resp_header_len;

    if((!l_recv_file_buf) || (l_recv_file_buf_len == 0) || (!l_recv_header_buf) || (l_recv_header_buf_len == 0))
    {
        printk("error @ %d\r\n", __LINE__);
        return 0;
    }
#if DEBUG_HTTP_TIME
    uint64_t tim = sysctl_get_time_us();
#endif
    //receive
    int per_packet_len = 0, buff_len = 0, last_buf_len = 0;
    int resp_body_start = 0, flag = 1, poll_times;
    uint32_t zero_len_cnt = 0, per_buf_len = 0;

    /*deal with all receive packet*/
    while(flag)
    {
        memset(l_tmp_buf, 0, BUFFSIZE);

        poll_times = 500;
        last_buf_len = 9999;
        do
        {
            buff_len = WiFiSpiClient_available(sock);
            // printk("buff_len:%d\r\n", buff_len);
#if 0
            if ((buff_len > 1024) || (last_buf_len == buff_len))
            {
                break;
            }
            if (buff_len > 0)
                last_buf_len = buff_len;
#else
            if(buff_len > 0)
            {
                zero_len_cnt = 0;
                break;
            }
#endif
            //FIXME:如果连续4*25=100ms 都是0，去查询一下连接状态
            zero_len_cnt++;
            if(zero_len_cnt > 25)
            {
                if(CLOSED == WiFiSpiClient_status(sock))
                {
                    break;
                } else
                {
                    zero_len_cnt = 0;
                }
            }

            msleep(4);
        } while(--poll_times > 0);

        // printk("buff_len:%d\r\n", buff_len);

        if(buff_len == 0)
        {
            /* receive error or no data*/
            flag = 0;
            printk("%s\r\n", l_recv_len ? "receive over" : "receive failed");
        } else if(buff_len > 0 && !resp_body_start)
        {
            per_buf_len = buff_len;
            per_packet_len = (per_buf_len > BUFFSIZE) ? BUFFSIZE : per_buf_len;
            WiFiSpiClient_read_buf(sock, l_tmp_buf, per_packet_len);
            /*deal with response header*/
            resp_body_start = read_past_header(l_tmp_buf, per_packet_len);

            per_buf_len -= per_packet_len;
            //如果没有接受完成
            if(per_buf_len)
            {
                do
                {
                    per_packet_len = (per_buf_len > BUFFSIZE) ? BUFFSIZE : per_buf_len;
                    WiFiSpiClient_read_buf(sock, l_tmp_buf, per_packet_len);
                    memcpy(l_recv_file_buf + l_recv_len, l_tmp_buf, per_packet_len);
                    l_recv_len += per_packet_len;
                    per_buf_len -= per_packet_len;
                } while(per_buf_len);
            }
        } else if(buff_len > 0 && resp_body_start)
        {
            per_buf_len = buff_len;
            do
            {
                per_packet_len = (per_buf_len > BUFFSIZE) ? BUFFSIZE : per_buf_len;
                WiFiSpiClient_read_buf(sock, l_tmp_buf, per_packet_len);
                memcpy(l_recv_file_buf + l_recv_len, l_tmp_buf, per_packet_len);
                l_recv_len += per_packet_len;
                per_buf_len -= per_packet_len;
            } while(per_buf_len);
        }

        if(l_recv_len > l_recv_file_buf_len)
        {
            printk("l_recv_len > l_recv_file_buf_len\r\n");
            flag = 0;
        }
    }
#if DEBUG_HTTP_TIME
    printk("http get result us %ld us\r\n", sysctl_get_time_us() - tim);
#endif
    WiFiSpiClient_stop(sock);

    return l_recv_len;
}