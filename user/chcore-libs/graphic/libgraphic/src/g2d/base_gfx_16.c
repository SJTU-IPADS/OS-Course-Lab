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
 * Implementation of 16-bit color graphics functions.
 */
#include "base_gfx.h"

#include "base_gfx_impl.h"
#include "blend.h"
#include "dc.h"

/* AND */
static inline void rop_srcand_16(RGB565 *dst, const RGB565 *src, void *arg)
{
        *(u16 *)dst &= *(u16 *)src;
}

/* COPY */
static inline void rop_srccopy_16(RGB565 *dst, const RGB565 *src, void *arg)
{
        *(u16 *)dst = *(u16 *)src;
}

/* INVERT + AND */
static inline void rop_srcerase_16(RGB565 *dst, const RGB565 *src, void *arg)
{
        *(u16 *)dst = ~*(u16 *)dst;
        *(u16 *)dst &= *(u16 *)src;
}

/* XOR */
static inline void rop_srcinvert_16(RGB565 *dst, const RGB565 *src, void *arg)
{
        *(u16 *)dst ^= *(u16 *)src;
}

/* OR */
static inline void rop_srcpaint_16(RGB565 *dst, const RGB565 *src, void *arg)
{
        *(u16 *)dst |= *(u16 *)src;
}

/* For transparent_blt */
static inline void transparent_pixel_16(RGB565 *dst, const RGB565 *src,
                                        RGB565 color)
{
        if (*(u16 *)src != *(u16 *)&color)
                *dst = *src;
}

static void draw_pixel_16(const PDC dc, int x, int y)
{
        RGB565 *ptr;
        ptr = (RGB565 *)get_dc_wr_ptr(dc, x, y);
        ptr[0] = rgb565_from_rgb(dc_paint_color(dc).rgb);
}

static void draw_hline_16(const PDC dc, int l, int r, int y)
{
        RGB565 *ptr_line = (RGB565 *)get_dc_wr_ptr(dc, l, y);
        RGB565 color = rgb565_from_rgb(dc_paint_color(dc).rgb);
        int w0 = (r - l + 1) / 4;
        int w1 = (r - l + 1) % 4;
        template_draw_hline(RGB565, ptr_line, w0, w1, color);
}

static void draw_vline_16(const PDC dc, int x, int t, int b)
{
        RGB565 *ptr_line = (RGB565 *)get_dc_wr_ptr(dc, x, t);
        RGB565 color = rgb565_from_rgb(dc_paint_color(dc).rgb);
        int h0 = (b - t + 1) / 4;
        int h1 = (b - t + 1) % 4;
        template_draw_vline(RGB565, dc, ptr_line, h0, h1, color);
}

static void draw_line_16(const PDC dc, int x1, int y1, int x2, int y2)
{
        RGB565 color = rgb565_from_rgb(dc_paint_color(dc).rgb);
        template_draw_line(RGB565, dc, x1, y1, x2, y2, color);
}

static void fill_rect_16(const PDC dc, int l, int t, int r, int b)
{
        RGB565 *ptr_line = (RGB565 *)get_dc_wr_ptr(dc, l, t);
        RGB565 color = rgb565_from_rgb(dc_fill_color(dc).rgb);
        int w0 = (r - l + 1) / 4;
        int w1 = (r - l + 1) % 4;
        int y;
        for (y = 0; y <= b - t; ++y) {
                template_draw_hline(RGB565, ptr_line, w0, w1, color);
                ptr_line = next_line(dc, ptr_line);
        }
}

static void draw_arc_16(const PDC dc, int x1, int y1, int x2, int y2, int x3,
                        int y3, int x4, int y4)
{
        // TODO...
}

static void draw_ellipse_16(const PDC dc, int l, int t, int r, int b)
{
        RGB565 color = rgb565_from_rgb(dc_paint_color(dc).rgb);
        template_draw_ellipse(RGB565, dc, l, t, r, b, color);
}

static void draw_char_16(const PDC dc, char ch, int x, int y, int l, int t,
                         int r, int b)
{
        RGB565 bg_color = rgb565_from_rgb(dc_bg_color(dc).rgb);
        RGB565 txt_color = rgb565_from_rgb(dc_txt_color(dc).rgb);
        template_draw_char(
                RGB565, dc, ch, x, y, l, t, r, b, bg_color, txt_color);
}

static void bitblt_16(const PDC dst_dc, int x1, int y1, const PDC src_dc,
                      int x2, int y2, int w, int h, enum BLT_OP bop, void *arg)
{
        u8 alpha;
        RGB565 color;

        switch (bop) {
        case BLT_BLEND:
                alpha = *(u8 *)arg;
                template_bitblt(RGB565,
                                dst_dc,
                                x1,
                                y1,
                                src_dc,
                                x2,
                                y2,
                                w,
                                h,
                                blend_pixel_16,
                                alpha);
                break;
        case BLT_TRANSPARENT:
                color = rgb565_from_rgb((*(RGBA *)arg).rgb);
                template_bitblt(RGB565,
                                dst_dc,
                                x1,
                                y1,
                                src_dc,
                                x2,
                                y2,
                                w,
                                h,
                                transparent_pixel_16,
                                color);
                break;
        default:
                break;
        }
}

static void stretch_blt_16(const PDC dst_dc, int x1, int y1, int sx1, int sy1,
                           const PDC src_dc, int x2, int y2, int sx2, int sy2,
                           int px, int py, int pw, int ph, enum BLT_OP bop,
                           void *arg)
{
        enum ROP rop;
        u8 alpha;
        RGB565 color;

        switch (bop) {
        case BLT_RASTER:
                rop = *(enum ROP *)arg;
                switch (rop) {
                case ROP_SRCAND:
                        template_stretch_blt(RGB565,
                                             dst_dc,
                                             x1,
                                             y1,
                                             sx1,
                                             sy1,
                                             src_dc,
                                             x2,
                                             y2,
                                             sx2,
                                             sy2,
                                             px,
                                             py,
                                             pw,
                                             ph,
                                             rop_srcand_16,
                                             NULL);
                        break;
                case ROP_SRCCOPY:
                        template_stretch_blt(RGB565,
                                             dst_dc,
                                             x1,
                                             y1,
                                             sx1,
                                             sy1,
                                             src_dc,
                                             x2,
                                             y2,
                                             sx2,
                                             sy2,
                                             px,
                                             py,
                                             pw,
                                             ph,
                                             rop_srccopy_16,
                                             NULL);
                        break;
                case ROP_SRCERASE:
                        template_stretch_blt(RGB565,
                                             dst_dc,
                                             x1,
                                             y1,
                                             sx1,
                                             sy1,
                                             src_dc,
                                             x2,
                                             y2,
                                             sx2,
                                             sy2,
                                             px,
                                             py,
                                             pw,
                                             ph,
                                             rop_srcerase_16,
                                             NULL);
                        break;
                case ROP_SRCINVERT:
                        template_stretch_blt(RGB565,
                                             dst_dc,
                                             x1,
                                             y1,
                                             sx1,
                                             sy1,
                                             src_dc,
                                             x2,
                                             y2,
                                             sx2,
                                             sy2,
                                             px,
                                             py,
                                             pw,
                                             ph,
                                             rop_srcinvert_16,
                                             NULL);
                        break;
                case ROP_SRCPAINT:
                        template_stretch_blt(RGB565,
                                             dst_dc,
                                             x1,
                                             y1,
                                             sx1,
                                             sy1,
                                             src_dc,
                                             x2,
                                             y2,
                                             sx2,
                                             sy2,
                                             px,
                                             py,
                                             pw,
                                             ph,
                                             rop_srcpaint_16,
                                             NULL);
                        break;
                default:
                        break;
                }
                break;
        case BLT_BLEND:
                alpha = *(u8 *)arg;
                template_stretch_blt(RGB565,
                                     dst_dc,
                                     x1,
                                     y1,
                                     sx1,
                                     sy1,
                                     src_dc,
                                     x2,
                                     y2,
                                     sx2,
                                     sy2,
                                     px,
                                     py,
                                     pw,
                                     ph,
                                     blend_pixel_16,
                                     alpha);
                break;
        case BLT_TRANSPARENT:
                color = rgb565_from_rgb((*(RGBA *)arg).rgb);
                template_stretch_blt(RGB565,
                                     dst_dc,
                                     x1,
                                     y1,
                                     sx1,
                                     sy1,
                                     src_dc,
                                     x2,
                                     y2,
                                     sx2,
                                     sy2,
                                     px,
                                     py,
                                     pw,
                                     ph,
                                     transparent_pixel_16,
                                     color);
                break;
        default:
                break;
        }
}

static struct base_gfx base_gfx_16 = {.draw_pixel = draw_pixel_16,
                                      .draw_hline = draw_hline_16,
                                      .draw_vline = draw_vline_16,
                                      .draw_line = draw_line_16,
                                      .fill_rect = fill_rect_16,
                                      .draw_arc = draw_arc_16,
                                      .draw_ellipse = draw_ellipse_16,
                                      .draw_char = draw_char_16,
                                      .bitblt = bitblt_16,
                                      .stretch_blt = stretch_blt_16};

inline struct base_gfx *get_base_gfx_16(void)
{
        return &base_gfx_16;
}
