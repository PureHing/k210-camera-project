#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "unistd.h"
#include "sysctl.h"
#include "global_config.h"
#include "sleep.h"
#include "dvp.h"
#include "fpioa.h"
#include "iomem.h"
#include "plic.h"
#include "camera.h"
#include "syslog.h"

#if CONFIG_ENABLE_LCD
#include "nt35310.h"
#include "lcd.h"
#endif

#include "image_process.h"
#include "sd_image.h"


static const char *TAG = "MAIN";
static image_t origin_image,kpu_image, display_image;
volatile uint8_t g_dvp_finish_flag;

#if CONFIG_ENABLE_SD_CARD
static int frame = 0;
volatile uint8_t g_save_flag;
char filename[100];


#if CONFIG_KEY_SAVE
static void irq_key(void *gp)
{
    g_save_flag = 1;
}
#endif
#endif
#if CONFIG_ENABLE_LCD
static uint32_t time_ram[8 * 16 * 8 / 2];

void rgb888_to_lcd(uint8_t *src, uint16_t *dest, size_t width, size_t height)
{
    size_t chn_size = width * height;
    for (size_t i = 0; i < width * height; i++)
    {
        uint8_t r = src[i];
        uint8_t g = src[chn_size + i];
        uint8_t b = src[chn_size * 2 + i];

        uint16_t rgb = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
        size_t d_i = i % 2 ? (i - 1) : (i + 1);
        dest[d_i] = rgb;
    }
}
#endif

static int on_irq_dvp(void *ctx)
{
  if(dvp_get_interrupt(DVP_STS_FRAME_FINISH))
    {
        dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
        dvp_clear_interrupt(DVP_STS_FRAME_FINISH);
        g_dvp_finish_flag = 1;

    } else
    {
        dvp_start_convert();
        dvp_clear_interrupt(DVP_STS_FRAME_START);
    }
    return 0;
}

int main(void)
{
    /* Set CPU and dvp clk */
    sysctl_pll_set_freq(SYSCTL_PLL0, 800000000UL);
    sysctl_pll_set_freq(SYSCTL_PLL1, 400000000UL);
    sysctl_clock_enable(SYSCTL_CLOCK_AI);

    plic_init();

    /* DVP init */
    LOGI(TAG, "DVP init");
    camera_init();
#if CONFIG_ENABLE_SD_CARD
    /* SD card init */
    if (sdcard_init())
    {
        LOGE(TAG, "Fail to init SD card");
        return -1;
    }
    if (fs_init())
    {
        LOGE(TAG, "Fail to mount file system,FAT32 err");
        return -1;
    }
#if CONFIG_KEY_SAVE
    gpiohs_set_drive_mode(CONFIG_KEY_BTN_NUM_GPIOHS, GPIO_DM_INPUT);
    gpiohs_set_pin_edge(CONFIG_KEY_BTN_NUM_GPIOHS, GPIO_PE_FALLING);
    gpiohs_set_irq(CONFIG_KEY_BTN_NUM_GPIOHS, 2, irq_key);

    FRESULT ret = FR_OK;
    char *dir = "SaveImage";

    ret = f_mkdir(dir);
    if (ret == FR_OK)
    {
        LOGD(TAG, "Mkdir %s ok", dir);
    }
    else if (ret == FR_EXIST)
    {
        LOGD(TAG, "Mkdir %s is exist", dir);
    }
    else
    {
        LOGE(TAG, "Mkdir %s err [%d]", dir, ret); // if exist
    }

#endif
#endif

#if CONFIG_ENABLE_LCD
    /* LCD init */
    LOGI(TAG, "LCD init");
    lcd_init();
#if CONFIG_MAIX_DOCK
    lcd_set_direction(DIR_YX_RLDU);
#else
    lcd_set_direction(DIR_YX_RLUD);
#endif

    lcd_clear(BLACK);
#endif

#if CONFIG_READ_IMG_SDCARD
    LOGI(TAG, "begin to read jpeg");
    uint64_t tm = sysctl_get_time_us();
    uint8_t *img = (uint8_t *)malloc(30000);
    sd_read_img("0:SaveImage/0.jpg", img, 30000);
    jpeg_decode_image_t *jpeg = pico_jpeg_decode(NULL, img, 30000, 1);
    jpeg_display(0, 0, jpeg);
    // convert_jpeg_img_order(jpeg);
    // lcd_draw_picture(0, 0, 320, 240, (uint32_t *)jpeg->img_data);
    LOGD(TAG, "decode use time %ld ms", (sysctl_get_time_us() - tm) / 1000);
    msleep(2000);
    free(jpeg->img_data);
    free(jpeg);
    free(img);
#endif
    kpu_image.pixel = 3;
    kpu_image.width = 320;
    kpu_image.height = 240;
    image_init(&kpu_image);
    display_image.pixel = 2;
    display_image.width = 320;
    display_image.height = 240;
    image_init(&display_image);

    dvp_set_ai_addr((uint32_t)kpu_image.addr, (uint32_t)(kpu_image.addr + 320 * 240),
                    (uint32_t)(kpu_image.addr + 320 * 240 * 2));
    dvp_set_display_addr((uint32_t)display_image.addr);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
    dvp_disable_auto();

    /* DVP interrupt config */
    LOGD(TAG, "DVP interrupt config");
    plic_set_priority(IRQN_DVP_INTERRUPT, 1);
    plic_irq_register(IRQN_DVP_INTERRUPT, on_irq_dvp, NULL);
    plic_irq_enable(IRQN_DVP_INTERRUPT);

    /* enable global interrupt */
    sysctl_enable_irq();
    
    /* system start */
    LOGI(TAG, "System start");
    uint64_t time_last = sysctl_get_time_us();
    uint64_t time_now = sysctl_get_time_us();
    char buf[10];
    
    while (1)
    {
        /* ai cal finish*/
        g_dvp_finish_flag = 0;
        dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
        dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);

        while(g_dvp_finish_flag == 0)
        {
        };
#if CONFIG_ENABLE_LCD
        /* display pic*/
        lcd_draw_picture(0, 0, 320, 240, (uint32_t *)display_image.addr);
#if CONFIG_KEY_SAVE

        if(g_save_flag && strcmp(CONFIG_IMAGE_SUFFIX, "bmp") == 0)
        {
            sprintf(filename, "0:SaveImage/%d.bmp", frame++);
            rgb565tobmp(display_image.addr, 320, 240, filename);
            LOGD(TAG, "save image [%s].", filename);
            lcd_ram_draw_string(filename, time_ram, BLACK, WHITE);
            lcd_draw_picture(0, 20, strlen(filename) * 8, 16, time_ram);
            g_save_flag = 0;
        }
        
        if(g_save_flag && strcmp(CONFIG_IMAGE_SUFFIX, "jpg") == 0)
        {
            sprintf(filename, "0:SaveImage/%d.jpg", frame++);
            if(!save_jpeg_sdcard(display_image.addr, filename, CONFIG_JPEG_COMPRESS_QUALITY))
            {
                frame--;
                sprintf(filename, "%s", "save failed");
            }

            lcd_ram_draw_string(filename, time_ram, BLACK, WHITE);
            lcd_draw_picture(0, 20, strlen(filename) * 8, 16, time_ram);
            g_save_flag = 0;
        }

#endif
        time_last = sysctl_get_time_us();
        float fps = 1e6 / (time_last - time_now);
        sprintf(buf, "%0.2ffps", fps);
        time_now = time_last;
        
        lcd_ram_draw_string(buf, time_ram, BLACK, WHITE);
        lcd_draw_picture(0, 0, strlen(buf) * 8, 16, time_ram);
#endif
    }
    image_deinit(&display_image);
    image_deinit(&kpu_image);
    return 0;
}
