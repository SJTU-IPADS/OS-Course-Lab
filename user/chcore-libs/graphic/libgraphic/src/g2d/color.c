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
 * Implementation of color functions.
 */
#include "g2d_internal.h"

inline u8 get_rgb565_blue(RGB565 rgb565)
{
        return ((u16)rgb565) & 0x1F;
}

inline u8 get_rgb565_green(RGB565 rgb565)
{
        return (((u16)rgb565) & 0x7E0) >> 5;
}

inline u8 get_rgb565_red(RGB565 rgb565)
{
        return ((u16)rgb565) >> 11;
}

/*
 * Convert RGB to RGB565.
 * Return RGB565.
 */
inline RGB565 rgb565_from_rgb(RGB color)
{
        RGB565 rgb565 = 0;
        rgb565 |= color.b >> 3; // b
        rgb565 |= (((u16)color.g) >> 2) << 5; // g
        rgb565 |= (((u16)color.r) >> 3) << 11; // r
        return rgb565;
}

/*
 * Convert RGB565 to RGB.
 * Return RGB.
 */
inline RGB rgb_from_rgb565(RGB565 rgb565)
{
        RGB rgb;
        rgb.b = get_rgb565_blue(rgb565);
        rgb.g = get_rgb565_green(rgb565);
        rgb.r = get_rgb565_red(rgb565);
        return rgb;
}

/*
 * Convert RGB565 to RGBA.
 * Return RGBA.
 */
inline RGBA rgba_from_rgb565(RGB565 rgb565)
{
        RGBA rgba;
        rgba.rgb = rgb_from_rgb565(rgb565);
        rgba.a = 0xFF;
        return rgba;
}

/*
 * Convert RGB565 and alpha to RGBA.
 * Return RGBA.
 */
inline RGBA rgba_from_rgb565_alpha(RGB565 rgb565, u8 alpha)
{
        RGBA rgba;
        rgba.rgb = rgb_from_rgb565(rgb565);
        rgba.a = alpha;
        return rgba;
}

/*
 * Convert u32 to RGBA.
 * Return RGBA.
 */
inline RGBA rgba_from_u32(u32 rgb)
{
        return (RGBA)(rgb | (0xFF << 24));
}

/*
 * Convert u32 and alpha to RGBA.
 * Return RGBA.
 */
inline RGBA rgba_from_u32_alpha(u32 rgb, u8 alpha)
{
        return (RGBA)(rgb | (alpha << 24));
}

/*
 * Convert RGB to RGBA.
 * Return RGBA.
 */
inline RGBA rgba_from_rgb(RGB rgb)
{
        RGBA rgba;
        rgba.rgb = rgb;
        rgba.a = 0xFF;
        return rgba;
}

/*
 * Convert RGB and alpha to RGBA.
 * Return RGBA.
 */
inline RGBA rgba_from_rgb_alpha(RGB rgb, u8 alpha)
{
        RGBA rgba;
        rgba.rgb = rgb;
        rgba.a = alpha;
        return rgba;
}
