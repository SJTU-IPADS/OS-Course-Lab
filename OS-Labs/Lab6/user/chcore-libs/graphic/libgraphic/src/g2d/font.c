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
 * Implementation of font.
 */
#include "font.h"

#include <malloc.h>
#include <string.h>

#include "dc.h"
#include "font8x16.h"
#include "region.h"

struct font {
        const char *fontbits_src;
        char *fontbits;
        int byte_w; // bit_w == byte_w * 8
        int byte_h; // bit_h == byte_h * 8
        int byte_size; // byte_w * byte_h * 8
        int byte_reverse; // for little-endian, etc
};

static PFONT default_bitfont = NULL;

PFONT _create_font(const char *fontbits_src, int byte_w, int byte_h,
                   int byte_reverse)
{
        PFONT font;
        int fontbits_size;
        char *cpy_dst;
        const char *cpy_src;
        int i, j;

        font = (PFONT)malloc(sizeof(*font));
        font->fontbits_src = fontbits_src;
        font->byte_w = byte_w;
        font->byte_h = byte_h;
        font->byte_size = byte_w * byte_h * 8;
        font->byte_reverse = byte_reverse;

        /* Copy fontbits */
        fontbits_size = font->byte_size * 256;
        font->fontbits = (char *)malloc(fontbits_size);
        if (byte_reverse <= 1) {
                memcpy(font->fontbits, font->fontbits_src, fontbits_size);
        } else {
                cpy_dst = font->fontbits;
                cpy_src = font->fontbits_src;
                for (i = 0; i < fontbits_size / byte_reverse; ++i) {
                        for (j = 0; j < byte_reverse; ++j) {
                                cpy_dst[byte_reverse - j - 1] = cpy_src[j];
                        }
                        cpy_dst += byte_reverse;
                        cpy_src += byte_reverse;
                }
        }

        return font;
}

void _del_font(PFONT font)
{
        if (font->fontbits != NULL)
                free(font->fontbits);
        free(font);
}

int _draw_char(const PDC dc, char ch, int x, int y)
{
        PRGN rgn0, rgn1;
        const RECT *rc;
        int i;

        if (dc_font(dc) == NULL)
                return -1;

        init_gdi_rgn_ex();
        rgn0 = get_gdi_rgn_ex0();
        rgn1 = get_gdi_rgn_ex1();

        /* Init rgn0 by target rect */
        _set_rc_rgn(rgn0,
                    x,
                    y,
                    x + dc_font_bit_width(dc) - 1,
                    y + dc_font_bit_height(dc) - 1);

        /* Cut rgn0 by DC rect */
        _set_rc_rgn(rgn1, 0, 0, dc_width(dc) - 1, dc_height(dc) - 1);
        _combine_rgn(rgn0, rgn0, rgn1, RGN_AND);
        if (_rgn_null(rgn0))
                return -1;

        /* Cut rgn0 by DC clip_rgn */
        if (dc_clip_rgn(dc) != NULL) {
                _combine_rgn(rgn0, rgn0, dc_clip_rgn(dc), RGN_AND);
                if (_rgn_null(rgn0))
                        return -1;
        }

        /* Draw char in rects of rgn0 */
        for (i = 0; i < rgn_rc_num(rgn0); ++i) {
                rc = rgn_rc(rgn0, i);
                dc_gfx(dc)->draw_char(dc,
                                      ch,
                                      x,
                                      y,
                                      rc->l - x,
                                      rc->t - y,
                                      rc->r - x,
                                      rc->b - y);
        }
        return 0;
}

PFONT _get_default_bitfont(void)
{
        if (default_bitfont == NULL)
                default_bitfont = _create_font(
                        (const char *)default_bitfont_8x16, 1, 2, 4);
        return default_bitfont;
}

inline const char *font_bits(const PFONT font)
{
        return font->fontbits;
}

inline int font_byte_width(const PFONT font)
{
        return font->byte_w;
}

inline int font_byte_height(const PFONT font)
{
        return font->byte_w;
}

inline int font_byte_size(const PFONT font)
{
        return font->byte_size;
}

inline int font_byte_reverse(const PFONT font)
{
        return font->byte_reverse;
}
