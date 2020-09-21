/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include "image_process.h"
#include "iomem.h"
int min(int a, int b){return a<b?a:b;}
/**
 * old kendryte sdk
*/
// int image_init(image_t *image)
// {
//     image->addr = calloc(image->width * image->height * image->pixel, 1);
//     if (image->addr == NULL)
//         return -1;
//     return 0;
// }

// void image_deinit(image_t *image)
// {
//     free(image->addr);
// }

int image_init(image_t *image)
{
    image->addr = iomem_malloc(image->width * image->height * image->pixel);
    if (image->addr == NULL)
        return -1;

    return 0;
}

void image_deinit(image_t *image)
{
    iomem_free(image->addr);
}

int image_crop(image_t *src, image_t *dst, int x, int y, int w, int h, int stride){
    int wh_dst = dst->width * dst->height;
    int wh_src = src->width * src->height;
    int channel = min(src->pixel, dst->pixel);
    if(channel==2){
        for(int j=y, dst_j=0; dst_j<h; j+=stride, dst_j++)
            for(int i=x, dst_i=0; dst_i<w; i+=stride, dst_i++)
                    ((uint16_t*)dst->addr)[dst_j * dst->width + dst_i] = ((uint16_t*)src->addr)[j * src->width + i];
        return 0;
    }
    for(int c=0; c < channel; c++)
        for(int j=y, dst_j=0; dst_j<h; j+=stride, dst_j++)
            for(int i=x, dst_i=0; dst_i<w; i+=stride, dst_i++)
                    dst->addr[c * wh_dst + dst_j * dst->width + dst_i] = src->addr[c * wh_src + j * src->width + i];
    return 0;
}

void image_resize(image_t *image_src, image_t *image_dst)
{
    uint16_t x1, x2, y1, y2;
    float w_scale, h_scale;
    float temp1, temp2;
    float x_src, y_src;

    uint8_t *r_src, *g_src, *b_src, *r_dst, *g_dst, *b_dst;
    uint16_t w_src, h_src, w_dst, h_dst;

    w_src = image_src->width;
    h_src = image_src->height;
    r_src = image_src->addr;
    g_src = r_src + w_src * h_src;
    b_src = g_src + w_src * h_src;
    w_dst = image_dst->width;
    h_dst = image_dst->height;
    r_dst = image_dst->addr;
    g_dst = r_dst + w_dst * h_dst;
    b_dst = g_dst + w_dst * h_dst;

    w_scale = (float)w_src / w_dst;
    h_scale = (float)h_src / h_dst;

    for (uint16_t y = 0; y < h_dst; y++)
    {
        for (uint16_t x = 0; x < w_dst; x++)
        {
            x_src = (x + 0.5f) * w_scale - 0.5f;
            x1 = (uint16_t)x_src;
            x2 = x1 + 1;
            y_src = (y + 0.5f) * h_scale - 0.5f;
            y1 = (uint16_t)y_src;
            y2 = y1 + 1;

            if (x2 >= w_src || y2 >= h_src)
            {   
                *(r_dst + x + y * w_dst) = *(r_src + x1 + y1 * w_src);
                *(g_dst + x + y * w_dst) = *(g_src + x1 + y1 * w_src);
                *(b_dst + x + y * w_dst) = *(b_src + x1 + y1 * w_src);
                continue;
            }

            temp1 = (x2 - x_src) * *(r_src + x1 + y1 * w_src) + (x_src - x1) * *(r_src + x2 + y1 * w_src);
            temp2 = (x2 - x_src) * *(r_src + x1 + y2 * w_src) + (x_src - x1) * *(r_src + x2 + y2 * w_src);
            *(r_dst + x + y * w_dst) = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
            temp1 = (x2 - x_src) * *(g_src + x1 + y1 * w_src) + (x_src - x1) * *(g_src + x2 + y1 * w_src);
            temp2 = (x2 - x_src) * *(g_src + x1 + y2 * w_src) + (x_src - x1) * *(g_src + x2 + y2 * w_src);
            *(g_dst + x + y * w_dst) = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
            temp1 = (x2 - x_src) * *(b_src + x1 + y1 * w_src) + (x_src - x1) * *(b_src + x2 + y1 * w_src);
            temp2 = (x2 - x_src) * *(b_src + x1 + y2 * w_src) + (x_src - x1) * *(b_src + x2 + y2 * w_src);
            *(b_dst + x + y * w_dst) = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
        }
    }
}