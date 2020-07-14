#ifndef __PICOJPEG_UTIL_H__
#define __PICOJPEG_UTIL_H__

#include <stdint.h>

typedef struct _jpeg_decode_image
{
    uint16_t width;
    uint16_t height;
    uint8_t comps;
    uint8_t scan_type;
    uint8_t *img_data;
} jpeg_decode_image_t;

void convert_jpeg_img_order(jpeg_decode_image_t *jpeg);
jpeg_decode_image_t *pico_jpeg_decode(uint8_t *out_img, uint8_t *buf, uint32_t buf_len, uint8_t rgb565);


void jpeg_display(uint16_t startx, uint16_t starty, jpeg_decode_image_t *jpeg);


#endif
