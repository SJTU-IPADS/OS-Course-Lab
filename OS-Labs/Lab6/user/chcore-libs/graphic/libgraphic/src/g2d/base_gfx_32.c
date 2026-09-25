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
 * Implementation of 32-bit color graphics functions.
 */
#include "base_gfx.h"

#include "base_gfx_impl.h"
#include "blend.h"
#include "dc.h"

/* AND */
static inline void rop_srcand_32(RGBA *dst, const RGBA *src, void *arg)
{
        *(u32 *)dst &= *(u32 *)src;
}

/* COPY */
static inline void rop_srccopy_32(RGBA *dst, const RGBA *src, void *arg)
{
        *(u32 *)dst = *(u32 *)src;
}

/* INVERT + AND */
static inline void rop_srcerase_32(RGBA *dst, const RGBA *src, void *arg)
{
        *(u32 *)dst = ~*(u32 *)dst;
        *(u32 *)dst &= *(u32 *)src;
}

/* XOR */
static inline void rop_srcinvert_32(RGBA *dst, const RGBA *src, void *arg)
{
        *(u32 *)dst ^= *(u32 *)src;
}

/* OR */
static inline void rop_srcpaint_32(RGBA *dst, const RGBA *src, void *arg)
{
        *(u32 *)dst |= *(u32 *)src;
}

/* For transparent_blt */
static inline void transparent_pixel_32(RGBA *dst, const RGBA *src, RGBA color)
{
        if (*(u32 *)src != *(u32 *)&color)
                *dst = *src;
}

static void draw_pixel_32(const PDC dc, int x, int y)
{
        RGBA *ptr;
        ptr = (RGBA *)get_dc_wr_ptr(dc, x, y);
        ptr[0] = dc_paint_color(dc);
}

static void draw_hline_32(const PDC dc, int l, int r, int y)
{
        RGBA *ptr_line = (RGBA *)get_dc_wr_ptr(dc, l, y);
        RGBA color = dc_paint_color(dc);
        int w0 = (r - l + 1) / 4;
        int w1 = (r - l + 1) % 4;
        template_draw_hline(RGBA, ptr_line, w0, w1, color);
}

static void draw_vline_32(const PDC dc, int x, int t, int b)
{
        RGBA *ptr_line = (RGBA *)get_dc_wr_ptr(dc, x, t);
        RGBA color = dc_paint_color(dc);
        int h0 = (b - t + 1) / 4;
        int h1 = (b - t + 1) % 4;
        template_draw_vline(RGBA, dc, ptr_line, h0, h1, color);
}

static void draw_line_32(const PDC dc, int x1, int y1, int x2, int y2)
{
        RGBA color = dc_paint_color(dc);
        template_draw_line(RGBA, dc, x1, y1, x2, y2, color);
}

static void fill_rect_32(const PDC dc, int l, int t, int r, int b)
{
        int i = 3072;
        RGBA *ptr_line = (RGBA *)get_dc_wr_ptr(dc, l, t);
        RGBA color = dc_fill_color(dc);
        int w0 = (r - l + 1) / 4;
        int w1 = (r - l + 1) % 4;
        int y;
        for (y = 0; y <= b - t; ++y) {
                template_draw_hline(RGBA, ptr_line, w0, w1, color);
                ptr_line = next_line(dc, ptr_line);
        }
}

static void draw_arc_32(const PDC dc, int x1, int y1, int x2, int y2, int x3,
                        int y3, int x4, int y4)
{
        // TODO...
}

static void draw_ellipse_32(const PDC dc, int l, int t, int r, int b)
{
        RGBA color = dc_paint_color(dc);
        template_draw_ellipse(RGBA, dc, l, t, r, b, color);
}

static void draw_char_32(const PDC dc, char ch, int x, int y, int l, int t,
                         int r, int b)
{
        RGBA bg_color = dc_bg_color(dc);
        RGBA txt_color = dc_txt_color(dc);
        template_draw_char(RGBA, dc, ch, x, y, l, t, r, b, bg_color, txt_color);
}

static void bitblt_32(const PDC dst_dc, int x1, int y1, const PDC src_dc,
                      int x2, int y2, int w, int h, enum BLT_OP bop, void *arg)
{
        u8 alpha;
        RGBA color;

        switch (bop) {
        case BLT_BLEND:
                alpha = *(u8 *)arg;
                template_bitblt(RGBA,
                                dst_dc,
                                x1,
                                y1,
                                src_dc,
                                x2,
                                y2,
                                w,
                                h,
                                blend_pixel_32,
                                alpha);
                break;
        case BLT_BLEND_RGBA:
                alpha = *(u8 *)arg;
                template_bitblt(RGBA,
                                dst_dc,
                                x1,
                                y1,
                                src_dc,
                                x2,
                                y2,
                                w,
                                h,
                                blend_pixel_rgba,
                                alpha);
                break;
        case BLT_TRANSPARENT:
                color = *(RGBA *)arg;
                template_bitblt(RGBA,
                                dst_dc,
                                x1,
                                y1,
                                src_dc,
                                x2,
                                y2,
                                w,
                                h,
                                transparent_pixel_32,
                                color);
                break;
        default:
                break;
        }
}

static void stretch_blt_32(const PDC dst_dc, int x1, int y1, int sx1, int sy1,
                           const PDC src_dc, int x2, int y2, int sx2, int sy2,
                           int px, int py, int pw, int ph, enum BLT_OP bop,
                           void *arg)
{
        enum ROP rop;
        u8 alpha;
        RGBA color;

        switch (bop) {
        case BLT_RASTER:
                rop = *(enum ROP *)arg;
                switch (rop) {
                case ROP_SRCAND:
                        template_stretch_blt(RGBA,
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
                                             rop_srcand_32,
                                             NULL);
                        break;
                case ROP_SRCCOPY:
                        template_stretch_blt(RGBA,
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
                                             rop_srccopy_32,
                                             NULL);
                        break;
                case ROP_SRCERASE:
                        template_stretch_blt(RGBA,
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
                                             rop_srcerase_32,
                                             NULL);
                        break;
                case ROP_SRCINVERT:
                        template_stretch_blt(RGBA,
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
                                             rop_srcinvert_32,
                                             NULL);
                        break;
                case ROP_SRCPAINT:
                        template_stretch_blt(RGBA,
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
                                             rop_srcpaint_32,
                                             NULL);
                        break;
                default:
                        break;
                }
                break;
        case BLT_BLEND:
                alpha = *(u8 *)arg;
                template_stretch_blt(RGBA,
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
                                     blend_pixel_32,
                                     alpha);
                break;
        case BLT_BLEND_RGBA:
                alpha = *(u8 *)arg;
                template_stretch_blt(RGBA,
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
                                     blend_pixel_rgba,
                                     alpha);
                break;
        case BLT_TRANSPARENT:
                color = *(RGBA *)arg;
                template_stretch_blt(RGBA,
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
                                     transparent_pixel_32,
                                     color);
                break;
        default:
                break;
        }
}

static struct base_gfx base_gfx_32 = {.draw_pixel = draw_pixel_32,
                                      .draw_hline = draw_hline_32,
                                      .draw_vline = draw_vline_32,
                                      .draw_line = draw_line_32,
                                      .fill_rect = fill_rect_32,
                                      .draw_arc = draw_arc_32,
                                      .draw_ellipse = draw_ellipse_32,
                                      .draw_char = draw_char_32,
                                      .bitblt = bitblt_32,
                                      .stretch_blt = stretch_blt_32};

inline struct base_gfx *get_base_gfx_32(void)
{
        return &base_gfx_32;
}
