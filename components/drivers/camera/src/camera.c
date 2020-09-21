#include "global_config.h"
#include "fpioa.h"
#include "sysctl.h"
#include "dvp.h"
#include "ov5640.h"
#include "ov2640.h"
#include "gc0328.h"
#include "cambus.h"
#include "syslog.h"
static const char *TAG = "camera_init";


static void io_mux_init(void)
{

#if CONFIG_MAIX_DOCK
    /* Init DVP IO map and function settings */
    fpioa_set_function(CONFIG_PIN_CMOS_RST, FUNC_CMOS_RST);
    fpioa_set_function(CONFIG_PIN_CMOS_PWDN, FUNC_CMOS_PWDN);
    fpioa_set_function(CONFIG_PIN_CMOS_XCLK, FUNC_CMOS_XCLK);
    fpioa_set_function(CONFIG_PIN_CMOS_VSYNC, FUNC_CMOS_VSYNC);
    fpioa_set_function(CONFIG_PIN_CMOS_HREF, FUNC_CMOS_HREF);
    fpioa_set_function(CONFIG_PIN_CMOS_PCLK, FUNC_CMOS_PCLK);
    fpioa_set_function(CONFIG_PIN_SCCB_SCLK, FUNC_SCCB_SCLK);
    fpioa_set_function(CONFIG_PIN_SCCB_SDA, FUNC_SCCB_SDA);
#if CONFIG_ENABLE_LCD
    /* Init SPI IO map and function settings */
    fpioa_set_function(CONFIG_PIN_NUM_LCD_DCX, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_LCD_DCX);
    fpioa_set_function(CONFIG_PIN_NUM_LCD_WRX, FUNC_SPI0_SS3);
    fpioa_set_function(CONFIG_PIN_NUM_LCD_SCK, FUNC_SPI0_SCLK);
    fpioa_set_function(CONFIG_PIN_NUM_LCD_RST, FUNC_GPIOHS0 + CONFIG_GPIOHS_NUM_LCD_RST);
#endif
    sysctl_set_spi0_dvp_data(1);
#if CONFIG_KEY_SAVE
    fpioa_set_function(CONFIG_KEY_BTN_NUM, FUNC_GPIOHS0 + CONFIG_KEY_BTN_NUM_GPIOHS);
#endif
#else

#endif
}

static void io_set_power(void)
{


#if CONFIG_MAIX_DOCK
    /* Set dvp and spi pin to 1.8V */
    sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);

#else
    /* Set dvp and spi pin to 1.8V */
    sysctl_set_power_mode(SYSCTL_POWER_BANK1, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK2, SYSCTL_POWER_V18);
#endif
}



void camera_init()
{
    io_mux_init();
    io_set_power();

#if CONFIG_CAMERA_OV5640
    dvp_init(16);
    dvp_set_xclk_rate(24000000);
    dvp_enable_burst();
    dvp_set_output_enable(0, 1);
    dvp_set_output_enable(1, 1);
    dvp_set_image_format(DVP_CFG_RGB_FORMAT);
    dvp_set_image_size(CONFIG_CAMERA_RESOLUTION_WIDTH, CONFIG_CAMERA_RESOLUTION_HEIGHT);

    ov5640_init();

#if 0
//    OV5640_Focus_Init();
    OV5640_Light_Mode(2);      //set auto
    OV5640_Color_Saturation(6); //default
    OV5640_Brightness(8);   //default
    OV5640_Contrast(3);     //default
//    OV5640_Sharpness(33);   //set auto
//    OV5640_Auto_Focus();
#endif

#elif CONFIG_CAMERA_OV2640
    dvp_init(8);
    dvp_set_xclk_rate(24000000);
    dvp_enable_burst();
    dvp_set_output_enable(0, 1);
    dvp_set_output_enable(1, 1);
    dvp_set_image_format(DVP_CFG_RGB_FORMAT);

    dvp_set_image_size(CONFIG_CAMERA_RESOLUTION_WIDTH, CONFIG_CAMERA_RESOLUTION_HEIGHT);
    ov2640_init();
#elif CONFIG_CAMERA_GC0328
    dvp_init(8);
    dvp_set_xclk_rate(24000000);
    dvp_enable_burst();
    dvp_set_output_enable(0, 1);
    dvp_set_output_enable(1, 1);
    dvp_set_image_format(DVP_CFG_YUV_FORMAT);
    dvp_set_image_size(320, 240);
    cambus_init(8, 2, 41, 40, 0, 0); //DVP SCL(41) SDA(40) pin ->software i2c

    gc0328_reset();
    gc0328_init();
#else
    LOGE(TAG,"not support camera type");
#endif
}