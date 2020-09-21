#ifndef __SD_IMAGE_H
#define __SD_IMAGE_H

#include <stdint.h>
#include "global_config.h"

#if CONFIG_ENABLE_SD_CARD

#include "ff.h"
#include "sdcard.h"
#if CONFIG_KEY_SAVE
#include "gpiohs.h"
#include "rgb2bmp.h"
#include "picojpeg.h"
#include "picojpeg_util.h"
#endif
int sdcard_init(void);
int fs_init(void);
FRESULT sd_read_img(TCHAR *path, uint8_t *src_img, int len);
int save_jpeg_sdcard(uint8_t *image_addr, const char *filename, int img_quality);
#endif

#endif