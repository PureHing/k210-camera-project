#include "net_8285.h"

//spi wifi
#include "WiFiSpi.h"
#include "myspi.h"

#include "global_config.h"
#include "camera.h"


#include "gpiohs.h"
#include "sleep.h"
#include "sysctl.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* for qrcode scan */
volatile uint8_t g_net_status = 0;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//0 ok
//1 module init failed
//2 module communication error
uint8_t spi_8266_init_device(void)
{
    //not connect to network
    g_net_status = 0;
    //hard reset
    gpiohs_set_pin(CONFIG_PIN_NUM_WIFI_EN, 0); //disable WIFI
    msleep(50);
    gpiohs_set_pin(CONFIG_PIN_NUM_WIFI_EN, 1); //enable WIFI
    msleep(500);

    my_spi_init();
    WiFiSpi_init(-1);

    // check for the presence of the ESP module:
    if (WiFiSpi_status() == WL_NO_SHIELD)
    {
 
        printk("WiFi module not present\r\n");
        // don't continue:
        return 1;
    }

    // WiFiSpi_softReset();

    printk("firmware version:%s\r\n", WiFiSpi_firmwareVersion());
    printk("protocol version : %s\r\n", WiFiSpi_protocolVersion());
    printk("status:%d\r\n", WiFiSpi_status());

    if (!WiFiSpi_checkProtocolVersion())
    {

        printk("Protocol version mismatch. Please upgrade the firmware\r\n");
        // don't continue:
        return 2;
    }

#if 1
    uint8_t mac[6];
    WiFiSpi_macAddress(mac);
#else
    uint8_t mac[6] = {0xDC, 0x4F, 0x22, 0x50, 0x97, 0xC3};
#endif

    return 0;
}

//1 connect ok
//0 connect timeout
uint8_t spi_8266_connect_ap(uint8_t wifi_ssid[32],
                            uint8_t wifi_passwd[32],
                            uint8_t timeout_n10s)
{
    uint8_t retry_cnt = 0;
    uint8_t status = WL_IDLE_STATUS; // the Wifi radio's status

    g_net_status = 0;

    while (status != WL_CONNECTED)
    {
        if (retry_cnt >= timeout_n10s)
        {
            return 0;
        }
        printk("Attempting to connect to WPA SSID: %s\r\n", wifi_ssid);
        status = WiFiSpi_begin_ssid_passwd(wifi_ssid, wifi_passwd);
        if (status == WL_CONNECTED)
            goto connect_success;
        retry_cnt++;
    }
connect_success:
    printk("You're connected to the network\r\n");
    g_net_status = 1;
    return 1;
}




