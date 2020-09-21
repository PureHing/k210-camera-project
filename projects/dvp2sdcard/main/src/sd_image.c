#include "sd_image.h"
#include "syslog.h"
#include "fpioa.h"
#include "sysctl.h"
#include "lcd.h"
#include "nt35310.h"
#include "jpeg_encode.h"
#include "picojpeg_util.h"

static const char *TAG = "SD_IMAGE";


#if CONFIG_ENABLE_JPEG_ENCODE
static uint8_t jpeg_buf[CONFIG_JPEG_BUF_LEN * 1024];
static jpeg_encode_t jpeg_src, jpeg_out;
#endif

#define SWAP_16(x) ((x >> 8 & 0xff) | (x << 8))

void jpeg_display(uint16_t startx, uint16_t starty, jpeg_decode_image_t *jpeg)
{
    uint16_t t[2];
    uint16_t *ptr = jpeg->img_data;
    uint16_t *p = ptr;

    lcd_set_area(startx, starty, startx + jpeg->width - 1, starty + jpeg->height - 1);

    for (uint32_t i = 0; i < (jpeg->width * jpeg->height); i += 2)
    {
        t[0] = *(ptr + 1);
        t[1] = *(ptr);
        *(ptr) = SWAP_16(t[0]);
        *(ptr + 1) = SWAP_16(t[1]);
        ptr += 2;
    }

    tft_write_word(p, (jpeg->width * jpeg->height / 2), 0);
}
#if CONFIG_ENABLE_SD_CARD
int sdcard_init(void)
{
    uint8_t status;

    fpioa_set_function(CONFIG_PIN_NUM_SD_CARD_SCLK, FUNC_SPI1_SCLK);
    fpioa_set_function(CONFIG_PIN_NUM_SD_CARD_MOSI, FUNC_SPI1_D0);
    fpioa_set_function(CONFIG_PIN_NUM_SD_CARD_MISO, FUNC_SPI1_D1);
    fpioa_set_function(CONFIG_PIN_NUM_SD_CARD_CS, FUNC_GPIOHS7);

    LOGI(TAG, "/******************sdcard test*****************/");
    status = sd_init();
    LOGD(TAG, "sd init %d", status);
    if (status != 0)
    {
        return status;
    }

    LOGD(TAG, "card info status %d", status);
    LOGD(TAG, "CardCapacity:%ld", cardinfo.CardCapacity);
    LOGD(TAG, "CardBlockSize:%d", cardinfo.CardBlockSize);
    return 0;
}

int fs_init(void)
{
    static FATFS sdcard_fs;
    FRESULT status;
    DIR dj;
    FILINFO fno;

    LOGI(TAG, "/********************fs test*******************/");
    status = f_mount(&sdcard_fs, _T("0:"), 1);
    LOGD(TAG, "mount sdcard:%d", status);
    if (status != FR_OK)
        return status;

    LOGI(TAG, "/----------------printf filename---------------/");
    status = f_findfirst(&dj, &fno, _T("0:"), _T("*"));
    while (status == FR_OK && fno.fname[0])
    {
        if (fno.fattrib & AM_DIR)
        {

            LOGD(TAG, "dir:%s", fno.fname);
        }
        else
        {
            LOGD(TAG, "file:%s", fno.fname);
        }
        status = f_findnext(&dj, &fno);
    }
    f_closedir(&dj);
    LOGI(TAG, "/----------------printf filename---------------/");
    return 0;
}

#if CONFIG_READ_IMG_SDCARD
FRESULT sd_read_img(TCHAR *path, uint8_t *src_img, int len)
{
    FIL file;
    FRESULT ret = FR_OK;
    uint32_t v_ret_len;
    if ((ret = f_open(&file, path, FA_READ)) == FR_OK)
    {
        ret = f_read(&file, (void *)src_img, len, &v_ret_len);
        if (ret != FR_OK)
        {
            LOGE(TAG, "read file error");
        }
        else
        {
            LOGD(TAG, "read file is %s", src_img);
        }
        f_close(&file);
    }
    return ret;
}
#endif
/**
 * rgb565 to jpeg
*/
#if CONFIG_ENABLE_JPEG_ENCODE
static int convert_image2jpeg(uint8_t *image, int Quality)
{

    uint64_t v_tim;
    v_tim = sysctl_get_time_us();

    jpeg_src.w = CONFIG_CAMERA_RESOLUTION_WIDTH;
    jpeg_src.h = CONFIG_CAMERA_RESOLUTION_HEIGHT;
    jpeg_src.bpp = 2;
    jpeg_src.data = image;

    jpeg_out.w = jpeg_src.w;
    jpeg_out.h = jpeg_src.h;
    jpeg_out.bpp = CONFIG_JPEG_BUF_LEN * 1024;
    jpeg_out.data = jpeg_buf;

    v_tim = sysctl_get_time_us();
    uint8_t ret = jpeg_compress(&jpeg_src, &jpeg_out, Quality, 0);
    if (ret == 0)
    {
        LOGD(TAG, "jpeg encode use %ld us", sysctl_get_time_us() - v_tim);
        LOGD(TAG, "w:%d\th:%d\tbpp:%d", jpeg_out.w, jpeg_out.h, jpeg_out.bpp);
        LOGD(TAG, "jpeg encode success!");
    }
    else
    {
        LOGE(TAG, "jpeg encode failed");
    }

    return ret;
}

int save_jpeg_sdcard(uint8_t *image_addr, const char *filename, int img_quality)
{
    FIL file;
    FRESULT ret = FR_OK;
    uint32_t ret_len = 0;

    if (convert_image2jpeg(image_addr, img_quality) == 0)
    {

        if ((ret = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK)
        {
            LOGD(TAG, "create file %s err[%d]", filename, ret);
            return ret;
        }

        f_write(&file, jpeg_out.data, jpeg_out.bpp, &ret_len);

        f_close(&file);
        LOGD(TAG, "Save jpeg image %s", filename);
        return 1;
    }
    else
    {
        return 0;
    }
}
#endif
#endif
