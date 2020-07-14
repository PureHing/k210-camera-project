#ifndef __YUV_TAB_H
#define __YUV_TAB_H

#include <stdio.h>

int8_t yuv_table(uint32_t idx);

void pix_fill_yuv(uint32_t idx, int8_t *y, int8_t *u, int8_t *v);

void pix_fill_8y(uint16_t *pixels, uint32_t ofs, int8_t *y);

void pix_fill_8uv2(uint16_t *pixels, uint32_t ofs, int8_t *u, int8_t *v);

void pix_fill_8yuv(uint16_t *pixels, uint32_t ofs, int8_t *y, int8_t *u, int8_t *v);

#endif
