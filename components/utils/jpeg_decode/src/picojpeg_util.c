//------------------------------------------------------------------------------
// jpg2tga.c
// JPEG to TGA file conversion example program.
// Public domain, Rich Geldreich <richgel99@gmail.com>
// Last updated Nov. 26, 2010
//------------------------------------------------------------------------------
#include "picojpeg.h"
#include "picojpeg_util.h"


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <math.h>

// #include "printf.h"
#define DEBUG_PRINTF(fmt, ...)  //printk(fmt, ##__VA_ARGS__)

/* clang-format off */
//------------------------------------------------------------------------------
#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif
#define RGB888_RED      0x00ff0000
#define RGB888_GREEN    0x0000ff00
#define RGB888_BLUE     0x000000ff
 
#define RGB565_RED      0xf800
#define RGB565_GREEN    0x07e0
#define RGB565_BLUE     0x001f
/* clang-format on */

unsigned short RGB888ToRGB565(unsigned int n888Color)
{
   unsigned short n565Color = 0;

   // 获取RGB单色，并截取高位
   unsigned char cRed = (n888Color & RGB888_RED) >> 19;
   unsigned char cGreen = (n888Color & RGB888_GREEN) >> 10;
   unsigned char cBlue = (n888Color & RGB888_BLUE) >> 3;

   // 连接
   n565Color = (cRed << 11) + (cGreen << 5) + (cBlue << 0);
   return n565Color;
}

unsigned int RGB565ToRGB888(unsigned short n565Color)
{
   unsigned int n888Color = 0;

   // 获取RGB单色，并填充低位
   unsigned char cRed = (n565Color & RGB565_RED) >> 8;
   unsigned char cGreen = (n565Color & RGB565_GREEN) >> 3;
   unsigned char cBlue = (n565Color & RGB565_BLUE) << 3;

   // 连接
   n888Color = (cRed << 16) + (cGreen << 8) + (cBlue << 0);
   return n888Color;
}
//------------------------------------------------------------------------------
typedef unsigned char uint8;
typedef unsigned int uint;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
typedef struct
{
   uint8_t *buf;
   uint size;
   uint curr;
} file_info_t;

//------------------------------------------------------------------------------
unsigned char pjpeg_need_bytes_callback(unsigned char *pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data)
{
   uint n;
   file_info_t *file = (file_info_t *)pCallback_data;

   n = min(file->size - file->curr, buf_size);
   memcpy(pBuf, file->buf + file->curr, n);
   *pBytes_actually_read = (unsigned char)(n);
   file->curr += n;
   return 0;
}

//------------------------------------------------------------------------------
// Loads JPEG image from specified file. Returns NULL on failure.
// On success, the malloc()'d image's width/height is written to *x and *y, and
// the number of components (1 or 3) is written to *comps.
// pScan_type can be NULL, if not it'll be set to the image's pjpeg_scan_type_t.
// Not thread safe.
// If reduce is non-zero, the image will be more quickly decoded at approximately
// 1/8 resolution (the actual returned resolution will depend on the JPEG
// subsampling factor).
uint8 *pjpeg_load_from_file(uint8_t *buf, uint32_t buf_len, int *x, int *y, int *comps, pjpeg_scan_type_t *pScan_type, int reduce, uint8_t rgb565, uint8_t *pixels, int *err)
{
   pjpeg_image_info_t image_info;
   int mcu_x = 0;
   int mcu_y = 0;
   uint row_pitch;
   uint8 *pImage;
   uint8 status;
   uint decoded_width, decoded_height;
   uint row_blocks_per_mcu, col_blocks_per_mcu;
   uint8_t one_color_size;
   file_info_t f_info;

   *x = 0;
   *y = 0;
   *comps = 0;
   if (pScan_type)
      *pScan_type = PJPG_GRAYSCALE;
   *err = 0;

   f_info.buf = buf;
   f_info.size = buf_len;
   f_info.curr = 0;

   status = pjpeg_decode_init(&image_info, pjpeg_need_bytes_callback, &f_info, (unsigned char)reduce);

   if (status)
   {
      DEBUG_PRINTF("pjpeg_decode_init() failed with status %u\r\n", status);
      if (status == PJPG_UNSUPPORTED_MODE)
      {
         DEBUG_PRINTF("Progressive JPEG files are not supported.\r\n");
      }
      return NULL;
   }

   DEBUG_PRINTF("reduce:%d\r\n", reduce);
   DEBUG_PRINTF("rgb565:%d\r\n", rgb565);

   if (pScan_type)
      *pScan_type = image_info.m_scanType;

   // In reduce mode output 1 pixel per 8x8 block.
   decoded_width = reduce ? (image_info.m_MCUSPerRow * image_info.m_MCUWidth) / 8 : image_info.m_width;
   decoded_height = reduce ? (image_info.m_MCUSPerCol * image_info.m_MCUHeight) / 8 : image_info.m_height;

   if ((image_info.m_comps != 1) && rgb565) //RGB565
      one_color_size = 2;
   else // grayscale or RGB888
      one_color_size = image_info.m_comps;

   row_pitch = decoded_width * one_color_size;
   if (pixels == NULL)
   {
      pImage = (uint8 *)malloc(row_pitch * decoded_height);
   }
   else
   {
      if ((row_pitch * decoded_height) > (320 * 240 * 3))
      {
         DEBUG_PRINTF("image: max supported size: 320 x 240\r\n");
         *err = -1;
         return NULL;
      }
      pImage = pixels;
   }
   if (!pImage)
   {
      *err = -2;
      return NULL;
   }

   row_blocks_per_mcu = image_info.m_MCUWidth >> 3;
   col_blocks_per_mcu = image_info.m_MCUHeight >> 3;

   for (;;)
   {
      int y, x;
      uint8 *pDst_row;
      status = pjpeg_decode_mcu();
      if (status)
      {
         if (status != PJPG_NO_MORE_BLOCKS)
         {
            DEBUG_PRINTF("pjpeg_decode_mcu() failed with status %u\r\n", status);
            if (pixels == NULL)
               free(pImage);
            *err = -1;
            return NULL;
         }
         break;
      }

      if (mcu_y >= image_info.m_MCUSPerCol)
      {
         if (pixels == NULL)
            free(pImage);
         *err = -3;
         return NULL;
      }

      if (reduce)
      {
         // In reduce mode, only the first pixel of each 8x8 block is valid.
         pDst_row = pImage + mcu_y * col_blocks_per_mcu * row_pitch + mcu_x * row_blocks_per_mcu * one_color_size;
         if (image_info.m_scanType == PJPG_GRAYSCALE)
         {
            *pDst_row = image_info.m_pMCUBufR[0];
         }
         else
         {
            uint y, x;
            for (y = 0; y < col_blocks_per_mcu; y++)
            {
               uint src_ofs = (y * 128U);
               for (x = 0; x < row_blocks_per_mcu; x++)
               {
                  if (rgb565)
                  {
                     unsigned short color = RGB888ToRGB565(image_info.m_pMCUBufR[src_ofs] << 16 | image_info.m_pMCUBufG[src_ofs] << 8 | image_info.m_pMCUBufB[src_ofs]);
                     pDst_row[0] = color >> 8 & 0xff;
                     pDst_row[1] = color & 0xff;
                  }
                  else
                  {
                     pDst_row[0] = image_info.m_pMCUBufR[src_ofs];
                     pDst_row[1] = image_info.m_pMCUBufG[src_ofs];
                     pDst_row[2] = image_info.m_pMCUBufB[src_ofs];
                  }
                  pDst_row += one_color_size;
                  src_ofs += 64;
               }

               pDst_row += row_pitch - one_color_size * row_blocks_per_mcu;
            }
         }
      }
      else
      {
         // Copy MCU's pixel blocks into the destination bitmap.
         pDst_row = pImage + (mcu_y * image_info.m_MCUHeight) * row_pitch + (mcu_x * image_info.m_MCUWidth * one_color_size);

         for (y = 0; y < image_info.m_MCUHeight; y += 8)
         {
            const int by_limit = min(8, image_info.m_height - (mcu_y * image_info.m_MCUHeight + y));

            for (x = 0; x < image_info.m_MCUWidth; x += 8)
            {
               uint8 *pDst_block = pDst_row + x * one_color_size;

               // Compute source byte offset of the block in the decoder's MCU buffer.
               uint src_ofs = (x * 8U) + (y * 16U);
               const uint8 *pSrcR = image_info.m_pMCUBufR + src_ofs;
               const uint8 *pSrcG = image_info.m_pMCUBufG + src_ofs;
               const uint8 *pSrcB = image_info.m_pMCUBufB + src_ofs;

               const int bx_limit = min(8, image_info.m_width - (mcu_x * image_info.m_MCUWidth + x));

               if (image_info.m_scanType == PJPG_GRAYSCALE)
               {
                  int bx, by;
                  for (by = 0; by < by_limit; by++)
                  {
                     uint8 *pDst = pDst_block;

                     for (bx = 0; bx < bx_limit; bx++)
                        *pDst++ = *pSrcR++;

                     pSrcR += (8 - bx_limit);

                     pDst_block += row_pitch;
                  }
               }
               else
               {
                  int bx, by;
                  for (by = 0; by < by_limit; by++)
                  {
                     uint8 *pDst = pDst_block;

                     for (bx = 0; bx < bx_limit; bx++)
                     {
                        if (rgb565)
                        {
                           unsigned short color = RGB888ToRGB565(*pSrcR << 16 | *pSrcG << 8 | *pSrcB);
                           pDst[0] = color >> 8 & 0xff;
                           pDst[1] = color & 0xff;
                        }
                        else
                        {
                           pDst[0] = *pSrcR;
                           pDst[1] = *pSrcG;
                           pDst[2] = *pSrcB;
                        }
                        pSrcR++;
                        pSrcG++;
                        pSrcB++;
                        pDst += one_color_size;
                     }

                     pSrcR += (8 - bx_limit);
                     pSrcG += (8 - bx_limit);
                     pSrcB += (8 - bx_limit);

                     pDst_block += row_pitch;
                  }
               }
            }

            pDst_row += (row_pitch * 8);
         }
      }

      mcu_x++;
      if (mcu_x == image_info.m_MCUSPerRow)
      {
         mcu_x = 0;
         mcu_y++;
      }
   }
   *err = 0;

   *x = decoded_width;
   *y = decoded_height;
   *comps = image_info.m_comps;

   return pImage;
}

/**
//------------------------------------------------------------------------------
typedef struct image_compare_results_tag
{
   double max_err;
   double mean;
   double mean_squared;
   double root_mean_squared;
   double peak_snr;
} image_compare_results;

static void get_pixel(int *pDst, const uint8 *pSrc, int luma_only, int num_comps)
{
   int r, g, b;
   if (num_comps == 1)
   {
      r = g = b = pSrc[0];
   }
   else if (luma_only)
   {
      const int YR = 19595, YG = 38470, YB = 7471;
      r = g = b = (pSrc[0] * YR + pSrc[1] * YG + pSrc[2] * YB + 32768) / 65536;
   }
   else
   {
      r = pSrc[0];
      g = pSrc[1];
      b = pSrc[2];
   }
   pDst[0] = r;
   pDst[1] = g;
   pDst[2] = b;
}

// Compute image error metrics.
static void image_compare(image_compare_results *pResults, int width, int height, const uint8 *pComp_image, int comp_image_comps, const uint8 *pUncomp_image_data, int uncomp_comps, int luma_only)
{
   double hist[256];
   double sum = 0.0f, sum2 = 0.0f;
   double total_values;
   const uint first_channel = 0, num_channels = 3;
   int x, y;
   uint i;

   memset(hist, 0, sizeof(hist));

   for (y = 0; y < height; y++)
   {
      for (x = 0; x < width; x++)
      {
         uint c;
         int a[3];
         int b[3];

         get_pixel(a, pComp_image + (y * width + x) * comp_image_comps, luma_only, comp_image_comps);
         get_pixel(b, pUncomp_image_data + (y * width + x) * uncomp_comps, luma_only, uncomp_comps);

         for (c = 0; c < num_channels; c++)
            hist[labs(a[first_channel + c] - b[first_channel + c])]++;
      }
   }

   pResults->max_err = 0;

   for (i = 0; i < 256; i++)
   {
      double x;
      if (!hist[i])
         continue;
      if (i > pResults->max_err)
         pResults->max_err = i;
      x = i * hist[i];
      sum += x;
      sum2 += i * x;
   }

   // See http://bmrc.berkeley.edu/courseware/cs294/fall97/assignment/psnr.html
   total_values = width * height;

   pResults->mean = sum / total_values;
   pResults->mean_squared = sum2 / total_values;

   pResults->root_mean_squared = sqrt(pResults->mean_squared);

   if (!pResults->root_mean_squared)
      pResults->peak_snr = 1e+10f;
   else
      pResults->peak_snr = log10(255.0f / pResults->root_mean_squared) * 20.0f;
}
**/

//------------------------------------------------------------------------------

#define SWAP_16(x) ((x >> 8 & 0xff) | (x << 8))

void convert_jpeg_img_order(jpeg_decode_image_t *jpeg)
{
   uint16_t t[2];
   uint16_t *ptr = jpeg->img_data;

   for (uint32_t i = 0; i < (jpeg->width * jpeg->height); i += 2)
   {
      t[0] = *(ptr + 1);
      t[1] = *(ptr);
      *(ptr) = SWAP_16(t[0]);
      *(ptr + 1) = SWAP_16(t[1]);
      ptr += 2;
   }
}

//------------------------------------------------------------------------------

jpeg_decode_image_t *pico_jpeg_decode(uint8_t *out_img, uint8_t *buf, uint32_t buf_len, uint8_t rgb565)
{

   int width, height, comps, err;
   pjpeg_scan_type_t scan_type;

   uint8_t *img_data = pjpeg_load_from_file(buf, buf_len, &width, &height, &comps, &scan_type, 0, rgb565, out_img, &err);

   if (img_data == NULL)
   {
      DEBUG_PRINTF("Failed loading source image! err:%s\r\n", (err == -1) ? "EINVAL" : (err == -2) ? "ENOMEM" : "EPERM");
      return NULL;
   }

   jpeg_decode_image_t *jpeg = (jpeg_decode_image_t *)malloc(sizeof(jpeg_decode_image_t));

   jpeg->width = width;
   jpeg->height = height;
   jpeg->comps = comps;
   jpeg->scan_type = scan_type;
   jpeg->img_data = img_data;

   return jpeg;
}
