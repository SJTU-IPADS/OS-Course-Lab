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
 * Implementation of bitmap.
 */
#include "bitmap.h"

#include "dc.h"
#include "dibitmap.h"
#include "g2d_math.h"
#include "g2d_memop.h"
#include "region.h"
#include "../dep/os.h"

struct bitmap {
        int w; // width
        int h; // height
        int depth; // depth
        int pitch; // pitch
        cap_t pmo_cap;
        union {
                u8 *raw_image; // Pointer to raw byte format array
                RGB565 *ptr_rgb565; // Pointer to RGB565 format array
                RGB *ptr_rgb; // Pointer to RGB format array
                RGBA *ptr_rgba; // Pointer to RGBA format array
        };
};

static PDC local_dc = NULL;

static inline void init_local_dc(void)
{
        if (local_dc == NULL)
                local_dc = _create_dc(DC_MEM);
}

/*
 * w in byte
 */

/* AND */
static void bitblt_srcand(const PDC dst, u8 *dst_ptr, int w, int h,
                          const PDC src, u8 *src_ptr)
{
        int row;
        if (dc_type(dst) == DC_DEVICE) {
                for (row = 0; row < h; ++row) {
                        dev_memand(dst_ptr, src_ptr, w);
                        dst_ptr = next_line(dst, dst_ptr);
                        src_ptr = next_line(src, src_ptr);
                }
        } else {
                for (row = 0; row < h; ++row) {
                        com_memand(dst_ptr, src_ptr, w);
                        dst_ptr = next_line(dst, dst_ptr);
                        src_ptr = next_line(src, src_ptr);
                }
        }
}

/* COPY */
static void bitblt_srccopy(const PDC dst, u8 *dst_ptr, int w, int h,
                           const PDC src, u8 *src_ptr)
{
        int row;
        if (dc_type(dst) == DC_DEVICE) {
                for (row = 0; row < h; ++row) {
                        dev_memcpy(dst_ptr, src_ptr, w);
                        dst_ptr = next_line(dst, dst_ptr);
                        src_ptr = next_line(src, src_ptr);
                }
        } else {
                for (row = 0; row < h; ++row) {
                        com_memcpy(dst_ptr, src_ptr, w);
                        dst_ptr = next_line(dst, dst_ptr);
                        src_ptr = next_line(src, src_ptr);
                }
        }
}

/* INVERT + AND */
static void bitblt_srcerase(const PDC dst, u8 *dst_ptr, int w, int h,
                            const PDC src, u8 *src_ptr)
{
        int row;
        if (dc_type(dst) == DC_DEVICE) {
                for (row = 0; row < h; ++row) {
                        dev_memerase(dst_ptr, src_ptr, w);
                        dst_ptr = next_line(dst, dst_ptr);
                        src_ptr = next_line(src, src_ptr);
                }
        } else {
                for (row = 0; row < h; ++row) {
                        com_memerase(dst_ptr, src_ptr, w);
                        dst_ptr = next_line(dst, dst_ptr);
                        src_ptr = next_line(src, src_ptr);
                }
        }
}

/* XOR */
static void bitblt_srcinvert(const PDC dst, u8 *dst_ptr, int w, int h,
                             const PDC src, u8 *src_ptr)
{
        int row;
        if (dc_type(dst) == DC_DEVICE) {
                for (row = 0; row < h; ++row) {
                        dev_memxor(dst_ptr, src_ptr, w);
                        dst_ptr = next_line(dst, dst_ptr);
                        src_ptr = next_line(src, src_ptr);
                }
        } else {
                for (row = 0; row < h; ++row) {
                        com_memxor(dst_ptr, src_ptr, w);
                        dst_ptr = next_line(dst, dst_ptr);
                        src_ptr = next_line(src, src_ptr);
                }
        }
}

/* OR */
static void bitblt_srcpaint(const PDC dst, u8 *dst_ptr, int w, int h,
                            const PDC src, u8 *src_ptr)
{
        int row;
        if (dc_type(dst) == DC_DEVICE) {
                for (row = 0; row < h; ++row) {
                        dev_memor(dst_ptr, src_ptr, w);
                        dst_ptr = next_line(dst, dst_ptr);
                        src_ptr = next_line(src, src_ptr);
                }
        } else {
                for (row = 0; row < h; ++row) {
                        com_memor(dst_ptr, src_ptr, w);
                        dst_ptr = next_line(dst, dst_ptr);
                        src_ptr = next_line(src, src_ptr);
                }
        }
}

static void do_bitblt_rect(const PDC dst, int x1, int y1, const PDC src, int x2,
                           int y2, int w, int h, enum BLT_OP bop, void *arg)
{
        u8 *dst_ptr = get_dc_wr_ptr(dst, x1, y1);
        u8 *src_ptr = get_dc_wr_ptr(src, x2, y2);
        enum ROP rop;
        int bpp; // byte per pixel

        if (bop == BLT_RASTER) {
                rop = *(enum ROP *)arg;
                bpp = dc_depth(dst) / 8;
                switch (rop) {
                case ROP_SRCAND:
                        bitblt_srcand(dst, dst_ptr, w * bpp, h, src, src_ptr);
                        break;
                case ROP_SRCCOPY:
                        bitblt_srccopy(dst, dst_ptr, w * bpp, h, src, src_ptr);
                        break;
                case ROP_SRCERASE:
                        bitblt_srcerase(dst, dst_ptr, w * bpp, h, src, src_ptr);
                        break;
                case ROP_SRCINVERT:
                        bitblt_srcinvert(
                                dst, dst_ptr, w * bpp, h, src, src_ptr);
                        break;
                case ROP_SRCPAINT:
                        bitblt_srcpaint(dst, dst_ptr, w * bpp, h, src, src_ptr);
                        break;
                default:
                        break;
                }
        } else {
                dc_gfx(dst)->bitblt(dst, x1, y1, src, x2, y2, w, h, bop, arg);
        }
}

static PBITMAP create_dib_bitmap_16(const PDIB dib)
{
        int w = dib_width(dib);
        int h = dib_height(dib);
        u64 src_offset = (h - 1) * w;
        RGBA *src32 = (RGBA *)dib_data(dib) + src_offset;
        RGB *src24 = (RGB *)dib_data(dib) + src_offset;
        RGB565 *src16 = (RGB565 *)dib_data(dib) + src_offset;
        int row, col;
        PBITMAP bitmap;
        RGB565 *dst;

        bitmap = _create_bitmap(w, h, 16, w * 2);
        dst = _bitmap_ptr_rgb565(bitmap);
        switch (dib_depth(dib)) {
        case 32:
                for (row = 0; row < h; ++row) {
                        for (col = 0; col < w; ++col) {
                                dst[col] = rgb565_from_rgb(src32[col].rgb);
                        }
                        dst += w;
                        src32 -= w;
                }
                break;
        case 24:
                for (row = 0; row < h; ++row) {
                        for (col = 0; col < w; ++col) {
                                dst[col] = rgb565_from_rgb(src24[col]);
                        }
                        dst += w;
                        src24 -= w;
                }
                break;
        case 16:
                for (row = 0; row < h; ++row) {
                        com_memcpy(dst, src16, w * 2);
                        dst += w;
                        src16 -= w;
                }
                break;
        case 8:
                // TODO...
                break;
        case 4:
                // TODO...
                break;
        default:
                _del_bitmap(bitmap);
                return NULL;
        }

        return bitmap;
}

static PBITMAP create_dib_bitmap_24(const PDIB dib)
{
        int w = dib_width(dib);
        int h = dib_height(dib);
        u64 src_offset = (h - 1) * w;
        RGBA *src32 = (RGBA *)dib_data(dib) + src_offset;
        RGB *src24 = (RGB *)dib_data(dib) + src_offset;
        RGB565 *src16 = (RGB565 *)dib_data(dib) + src_offset;
        int row, col;
        PBITMAP bitmap;
        RGB *dst;

        bitmap = _create_bitmap(w, h, 24, w * 3);
        dst = _bitmap_ptr_rgb(bitmap);
        switch (dib_depth(dib)) {
        case 32:
                for (row = 0; row < h; ++row) {
                        for (col = 0; col < w; ++col) {
                                dst[col] = src32[col].rgb;
                        }
                        dst += w;
                        src32 -= w;
                }
                break;
        case 24:
                for (row = 0; row < h; ++row) {
                        com_memcpy(dst, src24, w * 3);
                        dst += w;
                        src24 -= w;
                }
                break;
        case 16:
                for (row = 0; row < h; ++row) {
                        for (col = 0; col < w; ++col) {
                                dst[col] = rgb_from_rgb565(src16[col]);
                        }
                        dst += w;
                        src16 -= w;
                }
                break;
        case 8:
                // TODO...
                break;
        case 4:
                // TODO...
                break;
        default:
                _del_bitmap(bitmap);
                return NULL;
        }

        return bitmap;
}

static PBITMAP create_dib_bitmap_32(const PDIB dib)
{
        int w = dib_width(dib);
        int h = dib_height(dib);
        u64 src_offset = (h - 1) * w;
        RGBA *src32 = (RGBA *)dib_data(dib) + src_offset;
        RGB *src24 = (RGB *)dib_data(dib) + src_offset;
        RGB565 *src16 = (RGB565 *)dib_data(dib) + src_offset;
        int row, col;
        PBITMAP bitmap;
        RGBA *dst;

        bitmap = _create_bitmap(w, h, 32, w * 4);
        dst = _bitmap_ptr_rgba(bitmap);
        switch (dib_depth(dib)) {
        case 32:
                for (row = 0; row < h; ++row) {
                        com_memcpy(dst, src32, w * 4);
                        dst += w;
                        src32 -= w;
                }
                break;
        case 24:
                for (row = 0; row < h; ++row) {
                        for (col = 0; col < w; ++col) {
                                dst[col] = rgba_from_rgb(src24[col]);
                        }
                        dst += w;
                        src24 -= w;
                }
                break;
        case 16:
                for (row = 0; row < h; ++row) {
                        for (col = 0; col < w; ++col) {
                                dst[col] = rgba_from_rgb565(src16[col]);
                        }
                        dst += w;
                        src16 -= w;
                }
                break;
        case 8:
                // TODO...
                break;
        case 4:
                // TODO...
                break;
        default:
                _del_bitmap(bitmap);
                return NULL;
        }

        return bitmap;
}

PBITMAP _create_bitmap(int w, int h, int depth, int pitch)
{
        PBITMAP bitmap = (PBITMAP)malloc(sizeof(*bitmap));
        bitmap->w = w;
        bitmap->h = h;
        bitmap->depth = depth;
        bitmap->pitch = pitch;
        bitmap->pmo_cap = -1;
        bitmap->raw_image = (u8 *)malloc(pitch * h);
        return bitmap;
}

PBITMAP _create_bitmap_from_pmo(int w, int h, int depth, int pitch, int pmo_cap)
{
        PBITMAP bitmap = (PBITMAP)malloc(sizeof(*bitmap));
        bitmap->w = w;
        bitmap->h = h;
        bitmap->depth = depth;
        bitmap->pitch = pitch;
        bitmap->pmo_cap = pmo_cap;
        bitmap->raw_image = (u8 *)chcore_auto_map_pmo(
                pmo_cap, pitch * h, VM_READ | VM_WRITE);
        return bitmap;
}

PBITMAP _create_bitmap_from_vaddr(int w, int h, int depth, int pitch,
                                  void *vaddr)
{
        PBITMAP bitmap = (PBITMAP)malloc(sizeof(*bitmap));
        bitmap->w = w;
        bitmap->h = h;
        bitmap->depth = depth;
        bitmap->pitch = pitch;
        bitmap->pmo_cap = -1;
        bitmap->raw_image = (u8 *)vaddr;
        return bitmap;
}

PBITMAP _create_compat_bitmap(const PDC dc, int w, int h)
{
        PBITMAP bitmap = (PBITMAP)malloc(sizeof(*bitmap));
        bitmap->w = w;
        bitmap->h = h;
        bitmap->depth = dc_depth(dc);
        bitmap->pitch = w * (bitmap->depth / 8);
        bitmap->pmo_cap = -1;
        bitmap->raw_image = (u8 *)malloc(bitmap->pitch * h);
        return bitmap;
}

void _del_bitmap(PBITMAP bitmap)
{
        if (bitmap->pmo_cap != -1) {
                chcore_auto_unmap_pmo(bitmap->pmo_cap,
                                      (u64)bitmap->raw_image,
                                      bitmap->pitch * bitmap->h);
        } else if (bitmap->raw_image != NULL)
                free(bitmap->raw_image);
        free(bitmap);
}

inline void _clear_bitmap(PBITMAP bitmap)
{
        if (bitmap == NULL)
                return;
        memset((void *)bitmap->raw_image, 0, bitmap->pitch * bitmap->h);
}

inline int _bitmap_w(const PBITMAP bitmap)
{
        return bitmap->w;
}

inline int _bitmap_h(const PBITMAP bitmap)
{
        return bitmap->h;
}

inline int _bitmap_depth(const PBITMAP bitmap)
{
        return bitmap->depth;
}

inline int _bitmap_pitch(const PBITMAP bitmap)
{
        return bitmap->pitch;
}

inline int _bitmap_pmo_cap(const PBITMAP bitmap)
{
        return bitmap->pmo_cap;
}

inline u8 *_bitmap_raw_image(const PBITMAP bitmap)
{
        return bitmap->raw_image;
}

inline RGB565 *_bitmap_ptr_rgb565(const PBITMAP bitmap)
{
        return bitmap->ptr_rgb565;
}

inline RGB *_bitmap_ptr_rgb(const PBITMAP bitmap)
{
        return bitmap->ptr_rgb;
}

inline RGBA *_bitmap_ptr_rgba(const PBITMAP bitmap)
{
        return bitmap->ptr_rgba;
}

int do_bitblt(const PDC dst, int x1, int y1, const PDC src, int x2, int y2,
              int w, int h, enum BLT_OP bop, void *arg)
{
        const RECT *rc;
        int i;
        int offx, offy;
        PRGN rgn0, rgn1;

        if (dc_depth(dst) != dc_depth(src))
                return -1;

        offx = x1 - x2;
        offy = y1 - y2;
        init_gdi_rgn_ex();
        rgn0 = get_gdi_rgn_ex0();
        rgn1 = get_gdi_rgn_ex1();

        /* Init rgn0 by target src rect */
        _set_rc_rgn(rgn0, x2, y2, x2 + w - 1, y2 + h - 1);

        /* Cut rgn0 by src DC rect */
        _set_dc_rgn(rgn1, src);
        _combine_rgn(rgn0, rgn0, rgn1, RGN_AND);
        if (_rgn_null(rgn0))
                return -1;

        /* Move rgn0 to dst-coord */
        _move_rgn(rgn0, offx, offy);

        /* Cut rgn0 by dst DC rect */
        _set_dc_rgn(rgn1, dst);
        _combine_rgn(rgn0, rgn0, rgn1, RGN_AND);
        if (_rgn_null(rgn0))
                return -1;

        /* Cut rgn0 by dst DC clip_rgn */
        if (dc_clip_rgn(dst) != NULL) {
                _combine_rgn(rgn0, rgn0, dc_clip_rgn(dst), RGN_AND);
                if (_rgn_null(rgn0))
                        return -1;
        }

        /* Bitblt rgn0 */
        for (i = 0; i < rgn_rc_num(rgn0); ++i) {
                rc = rgn_rc(rgn0, i);
                do_bitblt_rect(dst,
                               rc->l,
                               rc->t,
                               src,
                               rc->l - offx,
                               rc->t - offy,
                               rc->r - rc->l + 1,
                               rc->b - rc->t + 1,
                               bop,
                               arg);
        }

        return 0;
}

int do_stretch_blt(const PDC dst, int x1, int y1, int w1, int h1, const PDC src,
                   int x2, int y2, int w2, int h2, enum BLT_OP bop, void *arg)
{
        PRGN rgn0, rgn1;
        int i;
        const RECT *rc;
        int lcm_w, lcm_h;
        int ax1, ay1, ax2, ay2;
        int sx1, sy1, sx2, sy2;
        int px, py, pw, ph;

        if (dc_depth(dst) != dc_depth(src))
                return -1;

        init_gdi_rgn_ex();
        rgn0 = get_gdi_rgn_ex0();
        rgn1 = get_gdi_rgn_ex1();

        /* Init rgn0 by target dst rect */
        _set_rc_rgn(rgn0, x1, y1, x1 + w1 - 1, y1 + h1 - 1);

        /* Cut rgn0 by dst DC rect */
        _set_dc_rgn(rgn1, dst);
        _combine_rgn(rgn0, rgn0, rgn1, RGN_AND);
        if (_rgn_null(rgn0))
                return -1;

        /* Cut rgn0 by dst DC clip_rgn */
        if (dc_clip_rgn(dst) != NULL) {
                _combine_rgn(rgn0, rgn0, dc_clip_rgn(dst), RGN_AND);
                if (_rgn_null(rgn0))
                        return -1;
        }

        /* StretchBlt rgn0 */
        lcm_w = lcm(w1, w2);
        lcm_h = lcm(h1, h2);
        sx1 = lcm_w / w1;
        sy1 = lcm_h / h1;
        sx2 = lcm_w / w2;
        sy2 = lcm_h / h2;
        for (i = 0; i < rgn_rc_num(rgn0); ++i) {
                rc = rgn_rc(rgn0, i);

                ax1 = rc->l;
                ay1 = rc->t;

                px = (ax1 - x1) * sx1;
                py = (ay1 - y1) * sy1;
                pw = px + (rc->r - ax1 + 1) * sx1;
                ph = py + (rc->b - ay1 + 1) * sy1;

                ax2 = x2 + px / sx2;
                ay2 = y2 + py / sy2;

                dc_gfx(dst)->stretch_blt(dst,
                                         ax1,
                                         ay1,
                                         sx1,
                                         sy1,
                                         src,
                                         ax2,
                                         ay2,
                                         sx2,
                                         sy2,
                                         px,
                                         py,
                                         pw,
                                         ph,
                                         bop,
                                         arg);
        }

        return 0;
}

inline int _bitblt_blend(const PDC dst, int x1, int y1, int w, int h, const PDC src,
                   int x2, int y2, enum ROP rop)
{
        return do_bitblt(dst, x1, y1, src, x2, y2, w, h, BLT_BLEND, &rop);
}

/* Bit-block transfer */
inline int _bitblt(const PDC dst, int x1, int y1, int w, int h, const PDC src,
                   int x2, int y2, enum ROP rop)
{
        return do_bitblt(dst, x1, y1, src, x2, y2, w, h, BLT_RASTER, &rop);
}

inline int _stretch_blt(const PDC dst, int x1, int y1, int w1, int h1,
                        const PDC src, int x2, int y2, int w2, int h2,
                        enum ROP rop)
{
        if (w1 == w2 && h1 == h2)
                return do_bitblt(
                        dst, x1, y1, src, x2, y2, w1, h1, BLT_RASTER, &rop);
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
                                      BLT_RASTER,
                                      &rop);
}

inline int _stretch_bitmap(const PDC dst, int x1, int y1, int w1, int h1,
                           const PBITMAP bitmap, int x2, int y2, int w2, int h2,
                           enum ROP rop)
{
        init_local_dc();
        _dc_select_bitmap(local_dc, bitmap);
        return _stretch_blt(dst, x1, y1, w1, h1, local_dc, x2, y2, w2, h2, rop);
}

inline int _transparent_blt(const PDC dst, int x1, int y1, int w1, int h1,
                            const PDC src, int x2, int y2, int w2, int h2,
                            u32 color)
{
        if (w1 == w2 && h1 == h2)
                return do_bitblt(dst,
                                 x1,
                                 y1,
                                 src,
                                 x2,
                                 y2,
                                 w1,
                                 h1,
                                 BLT_TRANSPARENT,
                                 &color);
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
                                      BLT_TRANSPARENT,
                                      &color);
}

PBITMAP _create_dib_bitmap(const PDC dc, const PDIB dib)
{
        switch (dc_depth(dc)) {
        case 32:
                return create_dib_bitmap_32(dib);
        case 24:
                return create_dib_bitmap_24(dib);
        case 16:
                return create_dib_bitmap_16(dib);
        default:
                return NULL;
        }
}
