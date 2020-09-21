#include <stdio.h>
#include <string.h>
#include "unistd.h"
#include "sysctl.h"
#include "global_config.h"

#include "dvp.h"
#include "fpioa.h"
#include "iomem.h"
#include "plic.h"
#include "camera.h"
#include "rtc.h"
#include "syslog.h"

#if CONFIG_ENABLE_LCD
#include "nt35310.h"
#include "lcd.h"
#endif



static const char *TAG = "MAIN";

static uint32_t *g_lcd_gram0;
static uint32_t *g_lcd_gram1;
volatile uint8_t g_dvp_finish_flag;


volatile uint8_t g_ram_mux;
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

/*GET TIME*/
void get_date_time()
{
    char time[25];
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int w;
    rtc_timer_get(&year, &month, &day, &hour, &minute, &second);
    sprintf(time, "%02d:%02d:%02d", hour, minute, second);
    w = strlen(time) * 8;
#if CONFIG_ENABLE_LCD
    lcd_ram_draw_string(time, time_ram, BLACK, WHITE);
    lcd_draw_picture(110, 0, w, 16, time_ram);
#endif
}
/*TIME INTERRUPT*/
int on_timer_interrupt()
{
    get_date_time();
    return 0;
}

static int on_irq_dvp(void *ctx)
{
    if (dvp_get_interrupt(DVP_STS_FRAME_FINISH))
    {
        /* switch gram */
        dvp_set_display_addr(g_ram_mux ? (uint32_t)g_lcd_gram0 : (uint32_t)g_lcd_gram1);
        
        dvp_clear_interrupt(DVP_STS_FRAME_FINISH);
        g_dvp_finish_flag = 1;
    }
    else
    {
        if (g_dvp_finish_flag == 0)
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


    plic_init();
    rtc_init(); 
    /* DVP init */
    LOGI(TAG, "DVP init");
    camera_init();
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
    g_lcd_gram0 = (uint32_t *)iomem_malloc(320 * 240 * 2);
    g_lcd_gram1 = (uint32_t *)iomem_malloc(320 * 240 * 2);


    dvp_set_ai_addr((uint32_t)0x40600000, (uint32_t)0x40612C00, (uint32_t)0x40625800);
    dvp_set_display_addr((uint32_t)g_lcd_gram0);
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
    g_ram_mux = 0;
    g_dvp_finish_flag = 0;
    dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);
    uint64_t time_last = sysctl_get_time_us();
    uint64_t time_now = sysctl_get_time_us();
    char buf[10];
    rtc_timer_set(2020, 7, 14, 14, 00, 50);

    while (1)
    {
        /* ai cal finish*/
        while (g_dvp_finish_flag == 0)
            ;
        g_dvp_finish_flag = 0;
        // uint8_t* d=g_ram_mux?(uint16_t*)g_lcd_gram0:(uint16_t*)g_lcd_gram1;
        // for(int i=0;i<240;i++)
        // {
        //     for(int j=0;j<320;j++)
        //     {
        //         printf("%d ",d[i*320+j]);
        //     }
        //     printf("\n");
        // }
#if CONFIG_ENABLE_LCD
        /* display pic*/
        g_ram_mux ^= 0x01;
        
        lcd_draw_picture(0, 0, 320, 240, g_ram_mux ? g_lcd_gram0 : g_lcd_gram1);

        time_last = sysctl_get_time_us();
        float fps = 1e6 / (time_last - time_now);
        sprintf(buf, "%0.2ffps", fps);
        time_now = time_last;
        get_date_time(); //update time
        lcd_ram_draw_string(buf, time_ram, BLACK, WHITE);
        lcd_draw_picture(0, 0, strlen(buf) * 8, 16, time_ram);
        printf("enable lcd\n");
#endif
    }
    iomem_free(g_lcd_gram0);
    iomem_free(g_lcd_gram1);
    return 0;
}
