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
 * Implementation of GDI engine functions.
 * Just check validity of arguments and redirect to internal GDI functions.
 */
#include "g2d_internal.h"

#include "arc.h"
#include "bitmap.h"
#include "blend.h"
#include "brush.h"
#include "dc.h"
#include "dibitmap.h"
#include "font.h"
#include "line.h"
#include "pixel.h"
#include "polygon.h"
#include "region.h"

/*
 * Arc
 */

inline int draw_arc(const PDC dc, int x1, int y1, int x2, int y2, int x3,
                    int y3, int x4, int y4)
{
        if (dc == NULL)
                return -1;
        return _draw_arc(dc, x1, y1, x2, y2, x3, y3, x4, y4);
}

inline int draw_ellipse(const PDC dc, int left, int top, int right, int bottom)
{
        if (dc == NULL)
                return -1;
        return _draw_ellipse(dc, left, top, right, bottom);
}

/*
 * Bitmap
 */

inline PBITMAP create_bitmap(int width, int height, int depth, int pitch)
{
        return _create_bitmap(width, height, depth, pitch);
}

inline PBITMAP create_bitmap_from_pmo(int width, int height, int depth,
                                      int pitch, int pmo_cap)
{
        return _create_bitmap_from_pmo(width, height, depth, pitch, pmo_cap);
}

inline PBITMAP create_bitmap_from_vaddr(int width, int height, int depth,
                                        int pitch, void *vaddr)
{
        return _create_bitmap_from_vaddr(width, height, depth, pitch, vaddr);
}

inline PBITMAP create_compat_bitmap(const PDC dc, int width, int height)
{
        return _create_compat_bitmap(dc, width, height);
}

inline void delete_bitmap(PBITMAP bitmap)
{
        if (bitmap == NULL)
                return;
        _del_bitmap(bitmap);
}

inline void clear_bitmap(PBITMAP bitmap)
{
        if (bitmap == NULL)
                return;
        _clear_bitmap(bitmap);
}

inline int get_bitmap_pmo_cap(const PBITMAP bitmap)
{
        if (bitmap == NULL)
                return -1;
        return _bitmap_pmo_cap(bitmap);
}

inline int get_bitmap_width(const PBITMAP bitmap)
{
        if (bitmap == NULL)
                return -1;
        return _bitmap_w(bitmap);
}

inline int get_bitmap_height(const PBITMAP bitmap)
{
        if (bitmap == NULL)
                return -1;
        return _bitmap_h(bitmap);
}

inline int get_bitmap_depth(const PBITMAP bitmap)
{
        if (bitmap == NULL)
                return -1;
        return _bitmap_depth(bitmap);
}

inline int get_bitmap_pitch(const PBITMAP bitmap)
{
        if (bitmap == NULL)
                return -1;
        return _bitmap_pitch(bitmap);
}

inline int bitblt(const PDC dst, int x1, int y1, int width, int height,
                  const PDC src, int x2, int y2, enum ROP rop)
{
        if (dst == NULL || dc_bitmap(dst) == NULL || src == NULL
            || dc_bitmap(src) == NULL)
                return -1;
        return _bitblt(dst, x1, y1, width, height, src, x2, y2, rop);
}

inline int stretch_blt(const PDC dst, int x1, int y1, int w1, int h1,
                       const PDC src, int x2, int y2, int w2, int h2,
                       enum ROP rop)
{
        if (dst == NULL || dc_bitmap(dst) == NULL || src == NULL
            || dc_bitmap(src) == NULL)
                return -1;
        return _stretch_blt(dst, x1, y1, w1, h1, src, x2, y2, w2, h2, rop);
}

inline int stretch_bitmap(const PDC dst, int x1, int y1, int w1, int h1,
                          const PBITMAP bitmap, int x2, int y2, int w2, int h2,
                          enum ROP rop)
{
        if (dst == NULL || dc_bitmap(dst) == NULL || bitmap == NULL)
                return -1;
        return _stretch_bitmap(
                dst, x1, y1, w1, h1, bitmap, x2, y2, w2, h2, rop);
}

inline int transparent_blt(const PDC dst, int x1, int y1, int w1, int h1,
                           const PDC src, int x2, int y2, int w2, int h2,
                           u32 color)
{
        if (dst == NULL || dc_bitmap(dst) == NULL || src == NULL
            || dc_bitmap(src) == NULL)
                return -1;
        return _transparent_blt(
                dst, x1, y1, w1, h1, src, x2, y2, w2, h2, color);
}

inline PBITMAP create_dib_bitmap(const PDC dc, const PDIB dib)
{
        if (dc == NULL || dib == NULL)
                return NULL;
        return _create_dib_bitmap(dc, dib);
}

/*
 * Blend
 */

inline int blend(const PDC dst, int x1, int y1, int w1, int h1, const PDC src,
                 int x2, int y2, int w2, int h2, u8 alpha)
{
        if (dst == NULL || dc_bitmap(dst) == NULL || src == NULL
            || dc_bitmap(src) == NULL)
            return -1;
        return _blend(dst, x1, y1, w1, h1, src, x2, y2, w2, h2, alpha);
}

inline int blend_rgba(const PDC dst, int x1, int y1, int w1, int h1,
                      const PDC src, int x2, int y2, int w2, int h2, u8 alpha)
{
        if (dst == NULL || src == NULL)
                return -1;
        return _blend_rgba(dst, x1, y1, w1, h1, src, x2, y2, w2, h2, alpha);
}

/*
 * Brush
 */

inline PBRUSH create_brush(void)
{
        return _create_brush();
}

inline void delete_brush(PBRUSH brush)
{
        if (brush == NULL)
                return;
        _del_brush(brush);
}

/*
 * DC
 */

inline PDC create_dc(enum DC_TYPE type)
{
        return _create_dc(type);
}

inline void delete_dc(PDC dc)
{
        if (dc == NULL)
                return;
        _del_dc(dc);
}

inline void dc_select_bitmap(PDC dc, const PBITMAP bitmap)
{
        if (dc == NULL || bitmap == NULL)
                return;
        _dc_select_bitmap(dc, bitmap);
}

inline void dc_select_clip_rgn(PDC dc, PRGN rgn)
{
        if (dc == NULL)
                return;
        _dc_select_clip_rgn(dc, rgn);
}

inline void dc_select_font(PDC dc, PFONT font)
{
        if (dc == NULL)
                return;
        _dc_select_font(dc, font);
}

inline void dc_release_clip_rgn(PDC dc)
{
        if (dc == NULL)
                return;
        _dc_release_clip_rgn(dc);
}

inline int get_dc_pmo_cap(const PDC dc)
{
        if (dc == NULL)
                return -1;
        return _get_dc_pmo_cap(dc);
}

PBRUSH get_dc_fill_brush(PDC dc)
{
        if (dc == NULL)
                return 0;
        return dc_fill_brush(dc);
}

PBRUSH get_dc_paint_brush(PDC dc)
{
        if (dc == NULL)
                return 0;
        return dc_paint_brush(dc);
}

PBRUSH get_dc_txt_brush(PDC dc)
{
        if (dc == NULL)
                return 0;
        return dc_txt_brush(dc);
}

PBRUSH get_dc_bg_brush(PDC dc)
{
        if (dc == NULL)
                return 0;
        return dc_bg_brush(dc);
}

int get_dc_width(const PDC dc)
{
        if (dc == NULL)
                return 0;
        return dc_width(dc);
}

int get_dc_height(const PDC dc)
{
        if (dc == NULL)
                return 0;
        return dc_height(dc);
}

int get_dc_font_bitwidth(const PDC dc)
{
        if (dc == NULL || dc_font(dc) == NULL)
                return 0;
        return dc_font_bit_width(dc);
}

int get_dc_font_bitheight(const PDC dc)
{
        if (dc == NULL || dc_font(dc) == NULL)
                return 0;
        return dc_font_bit_height(dc);
}

inline void set_dc_fill_brush(PDC dc, PBRUSH brush)
{
        if (dc == NULL || brush == NULL)
                return;
        _set_dc_fill_brush(dc, brush);
}

inline void set_dc_paint_brush(PDC dc, PBRUSH brush)
{
        if (dc == NULL || brush == NULL)
                return;
        _set_dc_paint_brush(dc, brush);
}

inline void set_dc_txt_brush(PDC dc, PBRUSH brush)
{
        if (dc == NULL || brush == NULL)
                return;
        _set_dc_txt_brush(dc, brush);
}

inline void set_dc_bg_brush(PDC dc, PBRUSH brush)
{
        if (dc == NULL || brush == NULL)
                return;
        _set_dc_bg_brush(dc, brush);
}

inline void set_dc_fill_color(PDC dc, RGBA color)
{
        if (dc == NULL)
                return;
        _set_dc_fill_color(dc, color);
}

inline void set_dc_paint_color(PDC dc, RGBA color)
{
        if (dc == NULL)
                return;
        _set_dc_paint_color(dc, color);
}

inline void set_dc_txt_color(PDC dc, RGBA color)
{
        if (dc == NULL)
                return;
        _set_dc_txt_color(dc, color);
}

inline void set_dc_bg_color(PDC dc, RGBA color)
{
        if (dc == NULL)
                return;
        _set_dc_bg_color(dc, color);
}

/*
 * DIBitmap
 */

inline PDIB create_dib(u32 width, u32 height, u32 depth)
{
        return _create_dib(width, height, depth);
}

inline void delete_dib(PDIB dib)
{
        if (dib == NULL)
                return;
        _del_dib(dib);
}

inline PDIB load_dib(const char *filename)
{
        if (filename == NULL)
                return NULL;
        return _load_dib(filename);
}

inline void save_dib(const PDIB dib, const char *filename)
{
        if (dib == NULL || filename == NULL)
                return;
        _save_dib(dib, filename);
}

inline u32 dib_get_width(const PDIB dib)
{
        if (dib == NULL)
                return 0;
        return _dib_get_width(dib);
}

inline u32 dib_get_height(const PDIB dib)
{
        if (dib == NULL)
                return 0;
        return _dib_get_height(dib);
}

inline u16 dib_get_depth(const PDIB dib)
{
        if (dib == NULL)
                return 0;
        return _dib_get_depth(dib);
}

inline void dib_get_pixel_rgb(const PDIB dib, u32 x, u32 y, u8 *r, u8 *g, u8 *b)
{
        if (dib == NULL)
                return;
        _dib_get_pixel_rgb(dib, x, y, r, g, b);
}

inline void dib_set_pixel_rgb(PDIB dib, u32 x, u32 y, u8 r, u8 g, u8 b)
{
        if (dib == NULL)
                return;
        _dib_set_pixel_rgb(dib, x, y, r, g, b);
}

inline void dib_get_pixel_index(const PDIB dib, u32 x, u32 y, u8 *val)
{
        if (dib == NULL || val == NULL)
                return;
        _dib_get_pixel_index(dib, x, y, val);
}

inline void dib_set_pixel_index(PDIB dib, u32 x, u32 y, u8 val)
{
        if (dib == NULL)
                return;
        _dib_set_pixel_index(dib, x, y, val);
}

inline void dib_get_palette_color(const PDIB dib, u8 index, u8 *r, u8 *g, u8 *b)
{
        if (dib == NULL)
                return;
        _dib_get_palette_color(dib, index, r, g, b);
}

inline void dib_set_palette_color(PDIB dib, u8 index, u8 r, u8 g, u8 b)
{
        if (dib == NULL)
                return;
        _dib_set_palette_color(dib, index, r, g, b);
}

/*
 * Font
 */

inline PFONT create_font(const char *bit_font, int byte_width, int byte_height,
                         int byte_reverse)
{
        return _create_font(bit_font, byte_width, byte_height, byte_reverse);
}

inline void delete_font(PFONT font)
{
        if (font == NULL)
                return;
        _del_font(font);
}

inline int draw_char(const PDC dc, char ch, int x, int y)
{
        if (dc == NULL)
                return -1;
        return _draw_char(dc, ch, x, y);
}

/*
 * Line
 */

inline int draw_hline(const PDC dc, int left, int right, int y)
{
        if (dc == NULL)
                return -1;
        return _draw_hline(dc, left, right, y);
}

inline int draw_vline(const PDC dc, int x, int top, int bottom)
{
        if (dc == NULL)
                return -1;
        return _draw_vline(dc, x, top, bottom);
}

inline int draw_line(const PDC dc, int x1, int y1, int x2, int y2)
{
        if (dc == NULL)
                return -1;
        return _draw_line(dc, x1, y1, x2, y2);
}

inline int draw_rect(const PDC dc, int left, int top, int right, int bottom)
{
        if (dc == NULL)
                return -1;
        return _draw_rect(dc, left, top, right, bottom);
}

inline int fill_rect(const PDC dc, int left, int top, int right, int bottom)
{
        if (dc == NULL)
                return -1;
        return _fill_rect(dc, left, top, right, bottom);
}

/*
 * Pixel
 */

inline int draw_pixel(const PDC dc, int x, int y)
{
        if (dc == NULL)
                return -1;
        return _draw_pixel(dc, x, y);
}

inline int draw_pixels(const PDC dc, const POINT *points, int nr_points)
{
        if (dc == NULL)
                return -1;
        if (points == NULL)
                return -1;
        return _draw_pixels(dc, points, nr_points);
}

/*
 * Polygon
 */

inline int draw_polygon(const PDC dc, const POINT *points, int nr_points)
{
        if (dc == NULL || points == NULL)
                return -1;
        return _draw_polygon(dc, points, nr_points);
}

inline int draw_polyline(const PDC dc, const POINT *points, int nr_points)
{
        if (dc == NULL || points == NULL)
                return -1;
        return _draw_polyline(dc, points, nr_points);
}

/*
 * Region
 */

inline PRGN create_region(int size)
{
        return _create_rgn(size);
}

inline int set_rect_region(PRGN rgn, int left, int top, int right, int bottom)
{
        if (rgn == NULL)
                return -1;
        return _set_rc_rgn(rgn, left, top, right, bottom);
}

inline int set_rc_region(PRGN rgn, const RECT *rc)
{
        if (rgn == NULL || rc == NULL)
                return -1;
        return _set_rc_rgn(rgn, rc->l, rc->t, rc->r, rc->b);
}

inline int set_dc_region(PRGN rgn, const PDC dc)
{
        if (rgn == NULL || dc == NULL)
                return -1;
        return _set_dc_rgn(rgn, dc);
}

inline void delete_region(PRGN rgn)
{
        if (rgn == NULL)
                return;
        _del_rgn(rgn);
}

inline RECT *get_region_rect(PRGN rgn, enum GET_RGN_RC_CMD cmd)
{
        if (rgn == NULL)
                return NULL;
        return _get_rgn_rc(rgn, cmd);
}

inline int combine_region(PRGN dst, const PRGN src1, const PRGN src2,
                          enum RGN_COMB_MODE mode)
{
        if (dst == NULL || src1 == NULL)
                return -1;
        return _combine_rgn(dst, src1, src2, mode);
}

inline void clear_region(PRGN rgn)
{
        if (rgn == NULL)
                return;
        _clear_rgn(rgn);
}

inline void move_region(PRGN rgn, int x, int y)
{
        if (rgn == NULL)
                return;
        _move_rgn(rgn, x, y);
}

inline bool pt_in_region(const PRGN rgn, int x, int y)
{
        if (rgn == NULL)
                return false;
        return _pt_in_rgn(rgn, x, y);
}

inline bool region_null(const PRGN rgn)
{
        if (rgn == NULL)
                return false;
        return _rgn_null(rgn);
}

inline void print_region(const PRGN rgn)
{
        if (rgn == NULL)
                return;
        _print_rgn(rgn);
}
