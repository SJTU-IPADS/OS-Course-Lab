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
 * Implementation of device context functions.
 */
#include "dc.h"

#include "base_gfx.h"
#include "bitmap.h"
#include "brush.h"
#include "font.h"
#include "region.h"
#include "../dep/os.h"

/* Device Context */
struct device_context {
        enum DC_TYPE type;

        /* Bitmap */
        PBITMAP bitmap;

        /* Clip Region */
        PRGN clip_rgn;

        /* Font */
        PFONT font;

        /* Brush */
        struct brush fill_brush;
        struct brush paint_brush;
        struct brush txt_brush;
        struct brush bg_brush;

        /* Base graphic functions */
        struct base_gfx *gfx;
};

PDC _create_dc(enum DC_TYPE type)
{
        PDC dc = (PDC)malloc(sizeof(*dc));

        /* Set common properties */
        dc->type = type;
        dc->bitmap = NULL;
        dc->clip_rgn = NULL;

        /* Set font to default bitfont */
        dc->font = _get_default_bitfont();

        /* Set brush color */
        dc->fill_brush.color = rgba_from_u32(COLOR_White);
        dc->paint_brush.color = rgba_from_u32(COLOR_White);
        dc->txt_brush.color = rgba_from_u32(COLOR_White);
        dc->bg_brush.color = rgba_from_u32(COLOR_White);

        /* Set graphic function ptr to NULL */
        dc->gfx = NULL;

        return dc;
}

inline void _del_dc(PDC dc)
{
        free(dc);
}

inline u8 *get_dc_wr_ptr(const PDC dc, int x, int y)
{
        return dc_raw_image(dc) + y * dc_pitch(dc) + x * (dc_depth(dc) / 8);
}

inline u8 *get_dc_start_ptr(const PDC dc)
{
        return dc_raw_image(dc);
}

inline u8 *get_dc_end_ptr(const PDC dc)
{
        return dc_raw_image(dc) + dc_pitch(dc) * dc_height(dc);
}

inline u8 *get_dc_line_left_ptr(const PDC dc, int y)
{
        return dc_raw_image(dc) + dc_pitch(dc) * y;
}

inline u8 *get_dc_line_right_ptr(const PDC dc, int y)
{
        return dc_raw_image(dc) + dc_pitch(dc) * y
               + (dc_width(dc) - 1) * (dc_depth(dc) / 8);
}

inline int _get_dc_pmo_cap(const PDC dc)
{
        return dc_pmo_cap(dc);
}

void _dc_select_bitmap(PDC dc, const PBITMAP bitmap)
{
        /* Set common properties */
        dc->bitmap = bitmap;

        /* Set graphic function ptr */
        switch (dc_depth(dc)) {
        case 32: /* 32 bit colour screen mode */
                dc->gfx = get_base_gfx_32();
                break;
        case 24: /* 24 bit colour screen mode */
                dc->gfx = get_base_gfx_24();
                break;
        case 16: /* 16 bit colour screen mode */
                dc->gfx = get_base_gfx_16();
                break;
        default:
                return;
        }
}

inline void _dc_select_clip_rgn(PDC dc, PRGN region)
{
        dc->clip_rgn = region;
}

inline void _dc_select_font(PDC dc, PFONT font)
{
        dc->font = font;
}

inline void _dc_release_clip_rgn(PDC dc)
{
        dc->clip_rgn = NULL;
}

inline void _set_dc_fill_brush(PDC dc, PBRUSH brush)
{
        dc->fill_brush = *brush;
}

inline void _set_dc_paint_brush(PDC dc, PBRUSH brush)
{
        dc->paint_brush = *brush;
}

inline void _set_dc_txt_brush(PDC dc, PBRUSH brush)
{
        dc->txt_brush = *brush;
}

inline void _set_dc_bg_brush(PDC dc, PBRUSH brush)
{
        dc->bg_brush = *brush;
}

inline void _set_dc_fill_color(PDC dc, RGBA color)
{
        dc->fill_brush.color = color;
}

inline void _set_dc_paint_color(PDC dc, RGBA color)
{
        dc->paint_brush.color = color;
}

inline void _set_dc_txt_color(PDC dc, RGBA color)
{
        dc->txt_brush.color = color;
}

inline void _set_dc_bg_color(PDC dc, RGBA color)
{
        dc->bg_brush.color = color;
}

bool pt_in_dc(const PDC dc, int x, int y)
{
        int i;

        if (x < 0 || x >= dc_width(dc) || y < 0 || y >= dc_height(dc))
                return false;
        if (dc->clip_rgn == NULL)
                return true;
        for (i = 0; i < rgn_rc_num(dc->clip_rgn); ++i) {
                if (pt_in_rect(rgn_rc(dc->clip_rgn, i), x, y))
                        return true;
        }
        return false;
}

inline enum DC_TYPE dc_type(const PDC dc)
{
        return dc->type;
}

inline PBITMAP dc_bitmap(const PDC dc)
{
        return dc->bitmap;
}

inline int dc_width(const PDC dc)
{
        return _bitmap_w(dc->bitmap);
}

inline int dc_height(const PDC dc)
{
        return _bitmap_h(dc->bitmap);
}

inline int dc_depth(const PDC dc)
{
        return _bitmap_depth(dc->bitmap);
}

inline int dc_pitch(const PDC dc)
{       
        return _bitmap_pitch(dc->bitmap);
}

inline int dc_pmo_cap(const PDC dc)
{
        return _bitmap_pmo_cap(dc->bitmap);
}

inline u8 *dc_raw_image(const PDC dc)
{
        return _bitmap_raw_image(dc->bitmap);
}

inline PRGN dc_clip_rgn(const PDC dc)
{
        return dc->clip_rgn;
}

inline PBRUSH dc_fill_brush(const PDC dc)
{
        return &dc->fill_brush;
}

inline PBRUSH dc_paint_brush(const PDC dc)
{
        return &dc->paint_brush;
}

inline PBRUSH dc_txt_brush(const PDC dc)
{
        return &dc->txt_brush;
}

inline PBRUSH dc_bg_brush(const PDC dc)
{
        return &dc->bg_brush;
}

inline RGBA dc_fill_color(const PDC dc)
{
        return dc->fill_brush.color;
}

inline RGBA dc_paint_color(const PDC dc)
{
        return dc->paint_brush.color;
}

inline RGBA dc_txt_color(const PDC dc)
{
        return dc->txt_brush.color;
}

inline RGBA dc_bg_color(const PDC dc)
{
        return dc->bg_brush.color;
}

inline PFONT dc_font(const PDC dc)
{
        return dc->font;
}

inline int dc_font_byte_width(const PDC dc)
{
        return font_byte_width(dc->font);
}

inline int dc_font_byte_height(const PDC dc)
{
        return font_byte_height(dc->font);
}

inline int dc_font_byte_size(const PDC dc)
{
        return font_byte_size(dc->font);
}

inline const char *dc_fontbits(const PDC dc)
{
        return font_bits(dc->font);
}

inline int dc_font_bit_width(const PDC dc)
{
        return font_byte_width(dc->font) * 8;
}

inline int dc_font_bit_height(const PDC dc)
{
        return font_byte_height(dc->font) * 8;
}

inline const struct base_gfx *dc_gfx(const PDC dc)
{
        return dc->gfx;
}

inline void *next_line(const PDC dc, void *ptr)
{
        return (void *)((u8 *)ptr + dc_pitch(dc));
}

inline void *prev_line(const PDC dc, void *ptr)
{
        return (void *)((u8 *)ptr - dc_pitch(dc));
}
