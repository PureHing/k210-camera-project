#include "demo_esp8285.h"
#include <string.h>
#include "net_8285.h"
#include <stdio.h>
#include "http_file.h"
    uint8_t wifi_ssid[32];
    uint8_t wifi_passwd[32];

void demo_esp8285(void)
{
    uint64_t tim = 0, last_mqtt_check_tim = 0;

    printf("demo_esp8285\r\n");

    /* init 8285 */
    spi_8266_init_device();

    if (strlen(wifi_ssid) > 0 && strlen(wifi_passwd) >= 8)
    {
        printf("ssid:%s\n",wifi_ssid);

        g_net_status = spi_8266_connect_ap(wifi_ssid, wifi_passwd, 2);
        if (g_net_status)
        {
            printf("wifi config %d \n",g_net_status);
        }
        else
        {

            printf("wifi config maybe error,we can not connect to it!\r\n");
        }
    }

    /*
        大致流程:
            开机判断是否有wifi的配置
                有正确的配置则进行联网
                没有正确的配置
                    提醒用户按下按键进行配网
                    然后扫描二维码
                    扫码获取到wifi的配置和进行联网操作
            联网成功就去执行http_get,http_post,mqtt的demo
    */
}

