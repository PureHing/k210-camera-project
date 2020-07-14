#ifndef __JPEG_COMPRESS_H
#define __JPEG_COMPRESS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* clang-format off */
#define bool uint8_t
#define false 0
#define true 1

// For K210
#define __CLZ(n) __builtin_clz(n)

// Software JPEG implementation.
#define FIX_0_382683433 ((int32_t)98)
#define FIX_0_541196100 ((int32_t)139)
#define FIX_0_707106781 ((int32_t)181)
#define FIX_1_306562965 ((int32_t)334)

#define DESCALE(x, y) (x >> y)
#define MULTIPLY(x, y) DESCALE((x) * (y), 8)

#define COLOR_GRAYSCALE_MIN 0
#define COLOR_GRAYSCALE_MAX 255

#define IM_LOG2_2(x)    (((x) &                0x2ULL) ? ( 2                        ) :             1) // NO ({ ... }) !
#define IM_LOG2_4(x)    (((x) &                0xCULL) ? ( 2 +  IM_LOG2_2((x) >>  2)) :  IM_LOG2_2(x)) // NO ({ ... }) !
#define IM_LOG2_8(x)    (((x) &               0xF0ULL) ? ( 4 +  IM_LOG2_4((x) >>  4)) :  IM_LOG2_4(x)) // NO ({ ... }) !
#define IM_LOG2_16(x)   (((x) &             0xFF00ULL) ? ( 8 +  IM_LOG2_8((x) >>  8)) :  IM_LOG2_8(x)) // NO ({ ... }) !
#define IM_LOG2_32(x)   (((x) &         0xFFFF0000ULL) ? (16 + IM_LOG2_16((x) >> 16)) : IM_LOG2_16(x)) // NO ({ ... }) !
#define IM_LOG2(x)      (((x) & 0xFFFFFFFF00000000ULL) ? (32 + IM_LOG2_32((x) >> 32)) : IM_LOG2_32(x)) // NO ({ ... }) !

#define UINT32_T_BITS   (sizeof(uint32_t) * 8)
#define UINT32_T_MASK   (UINT32_T_BITS - 1)
#define UINT32_T_SHIFT  IM_LOG2(UINT32_T_MASK)

#define IM_R825(p) \
    ({ __typeof__ (p) _p = (p); \
       rb825_table[_p]; })

#define IM_G826(p) \
    ({ __typeof__ (p) _p = (p); \
       g826_table[_p]; })

#define IM_B825(p) \
    ({ __typeof__ (p) _p = (p); \
       rb825_table[_p]; })

#define IM_RGB565(r, g, b) \
    ({ __typeof__ (r) _r = (r); \
       __typeof__ (g) _g = (g); \
       __typeof__ (b) _b = (b); \
       ((_r)<<3)|((_g)>>3)|((_g)<<13)|((_b)<<8); })

#define IMAGE_GET_BINARY_PIXEL(image, x, y) \
({ \
    __typeof__ (image) _image = (image); \
    __typeof__ (x) _x = (x); \
    __typeof__ (y) _y = (y); \
    (((uint32_t *) _image->data)[(((_image->w + UINT32_T_MASK) >> UINT32_T_SHIFT) * _y) + (_x >> UINT32_T_SHIFT)] >> (_x & UINT32_T_MASK)) & 1; \
})

#define IMAGE_GET_GRAYSCALE_PIXEL(image, x, y) \
({ \
    __typeof__ (image) _image = (image); \
    __typeof__ (x) _x = (x); \
    __typeof__ (y) _y = (y); \
    ((uint8_t *) _image->data)[(_image->w * _y) + _x]; \
})

#define IMAGE_GET_RGB565_PIXEL(image, x, y) \
({ \
    __typeof__ (image) _image = (image); \
    __typeof__ (x) _x = (x); \
    __typeof__ (y) _y = (y); \
    ((uint16_t *) _image->data)[(_image->w * _y) + _x]; \
})

#define IM_GET_RAW_PIXEL(img, x, y) \
    ({ __typeof__ (img) _img = (img); \
       __typeof__ (x) _x = (x); \
       __typeof__ (y) _y = (y); \
       ((uint8_t*)_img->pixels)[(_y*_img->w)+_x]; })

#define IM_GET_RAW_PIXEL_CHECK_BOUNDS_X(img, x, y) \
    ({ __typeof__ (img) _img = (img); \
       __typeof__ (x) _x = (x); \
       __typeof__ (y) _y = (y); \
       _x = (_x < 0) ? 0 : (_x >= img->w) ? (img->w -1): _x; \
       ((uint8_t*)_img->pixels)[(_y*_img->w)+_x]; })
       
#define IM_GET_RAW_PIXEL_CHECK_BOUNDS_Y(img, x, y) \
    ({ __typeof__ (img) _img = (img); \
       __typeof__ (x) _x = (x); \
       __typeof__ (y) _y = (y); \
       _y = (_y < 0) ? 0 : (_y >= img->h) ? (img->h -1): _y; \
       ((uint8_t*)_img->pixels)[(_y*_img->w)+_x]; })


#define IM_GET_RAW_PIXEL_CHECK_BOUNDS_XY(img, x, y) \
    ({ __typeof__ (img) _img = (img); \
       __typeof__ (x) _x = (x); \
       __typeof__ (y) _y = (y); \
       _x = (_x < 0) ? 0 : (_x >= img->w) ? (img->w -1): _x; \
       _y = (_y < 0) ? 0 : (_y >= img->h) ? (img->h -1): _y; \
       ((uint8_t*)_img->pixels)[(_y*_img->w)+_x]; })

#define COLOR_BINARY_TO_GRAYSCALE(pixel) ((pixel) * COLOR_GRAYSCALE_MAX)

/* clang-format on */

typedef struct
{
    int idx;
    int length;
    uint8_t *buf;
    int bitc, bitb;
    bool realloc;
    bool overflow;
} __attribute__((aligned(8))) jpeg_buf_t;

typedef enum  jpeg_subsample {
    JPEG_SUBSAMPLE_1x1 = 0x11,  // 1x1 chroma subsampling (No subsampling)
    JPEG_SUBSAMPLE_2x1 = 0x21,  // 2x2 chroma subsampling
    JPEG_SUBSAMPLE_2x2 = 0x22,  // 2x2 chroma subsampling
} __attribute__((aligned(8))) jpeg_subsample_t;

typedef struct _jpeg_encode
{
    int w;
    int h;
    int bpp;
    union {
        uint8_t *pixels;
        uint8_t *data;
    };
} __attribute__((aligned(8))) jpeg_encode_t;

bool jpeg_compress(jpeg_encode_t *src, jpeg_encode_t *dst, int quality, bool realloc);

//uint8_t reverse_u32pixel(uint32_t *addr, uint32_t length);

#endif
