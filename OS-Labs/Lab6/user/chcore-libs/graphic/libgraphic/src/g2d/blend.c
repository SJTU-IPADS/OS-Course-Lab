/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

/*
 * Implementation of blend functions.
 */
#include "blend.h"

#include "bitmap.h"
#include "dc.h"

void blend_pixel_16(RGB565 *dst, const RGB565 *src, u8 alpha)
{
        u32 r, g, b;
        RGB565 rgb565 = 0;

        b = get_rgb565_blue(*dst) * (255 - alpha)
            + get_rgb565_blue(*src) * alpha;
        b /= 255;
        g = get_rgb565_green(*dst) * (255 - alpha)
            + get_rgb565_green(*src) * alpha;
        g /= 255;
        r = get_rgb565_red(*dst) * (255 - alpha) + get_rgb565_red(*src) * alpha;
        r /= 255;

        rgb565 |= (u16)b; // blue
        rgb565 |= ((u16)g) << 5; // green
        rgb565 |= ((u16)r) << 11; // red
        *dst = rgb565;
}

void blend_pixel_24(RGB *dst, const RGB *src, u8 alpha)
{
        u32 r, g, b;
        RGB rgb;

        b = dst->b * (255 - alpha) + src->b * alpha;
        b /= 255;
        g = dst->g * (255 - alpha) + src->g * alpha;
        g /= 255;
        r = dst->r * (255 - alpha) + src->r * alpha;
        r /= 255;

        rgb.b = b;
        rgb.g = g;
        rgb.r = r;
        *dst = rgb;
}

void blend_pixel_32(RGBA *dst, const RGBA *src, u8 alpha)
{
        u32 r, g, b, a;
        RGBA rgba;

        b = dst->b * (255 - alpha) + src->b * alpha;
        b /= 255;
        g = dst->g * (255 - alpha) + src->g * alpha;
        g /= 255;
        r = dst->r * (255 - alpha) + src->r * alpha;
        r /= 255;
        a = dst->a * (255 - alpha) + src->a * alpha;
        a /= 255;

        rgba.b = b;
        rgba.g = g;
        rgba.r = r;
        rgba.a = a;
        *dst = rgba;
}

inline void blend_pixel_rgba(RGBA *dst, const RGBA *src, u8 alpha)
{
        u32 r, g, b, a;
        RGBA rgba;

        b = src->b;
        g = src->g;
        r = src->r;
        a = src->a;

        if (alpha != 255) {
                b = b * alpha / 255;
                g = g * alpha / 255;
                r = r * alpha / 255;
                a = a * alpha / 255;
        }

        b = dst->b * (255 - a) + b * a;
        b /= 255;
        g = dst->g * (255 - a) + g * a;
        g /= 255;
        r = dst->r * (255 - a) + r * a;
        r /= 255;
        a = dst->a * (255 - a) + a * a;
        a /= 255;

        rgba.b = b;
        rgba.g = g;
        rgba.r = r;
        rgba.a = a;
        *dst = rgba;
}

int _blend(const PDC dst, int x1, int y1, int w1, int h1, const PDC src, int x2,
           int y2, int w2, int h2, u8 alpha)
{
        if (w1 == w2 && h1 == h2)
                return do_bitblt(
                        dst, x1, y1, src, x2, y2, w1, h1, BLT_BLEND, &alpha);
        else
                return do_stretch_blt(dst,
                                      x1,
                                      y1,
                                      w1,
                                      h1,
                                      src,
                                      x2,
                                      y2,
                                      w2,
                                      h2,
                                      BLT_BLEND,
                                      &alpha);
}

int _blend_rgba(const PDC dst, int x1, int y1, int w1, int h1, const PDC src,
                int x2, int y2, int w2, int h2, u8 alpha)
{
        if (dc_depth(dst) != 32 || dc_depth(src) != 32)
                return -1;
        if (w1 == w2 && h1 == h2)
                return do_bitblt(dst,
                                 x1,
                                 y1,
                                 src,
                                 x2,
                                 y2,
                                 w1,
                                 h1,
                                 BLT_BLEND_RGBA,
                                 &alpha);
        else
                return do_stretch_blt(dst,
                                      x1,
                                      y1,
                                      w1,
                                      h1,
                                      src,
                                      x2,
                                      y2,
                                      w2,
                                      h2,
                                      BLT_BLEND_RGBA,
                                      &alpha);
}
