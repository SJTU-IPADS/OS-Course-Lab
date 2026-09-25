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
 * Bitmap header file.
 */
#pragma once

#include "base_gfx.h"
#include "g2d_internal.h"

/* DDB functions */
PBITMAP _create_bitmap(int w, int h, int depth, int pitch);
PBITMAP _create_bitmap_from_pmo(int w, int h, int depth, int pitch,
                                int pmo_cap);
PBITMAP _create_bitmap_from_vaddr(int w, int h, int depth, int pitch,
                                  void *vaddr);
PBITMAP _create_compat_bitmap(const PDC dc, int w, int h);
void _del_bitmap(PBITMAP bitmap);
void _clear_bitmap(PBITMAP bitmap);

int _bitmap_w(const PBITMAP bitmap);
int _bitmap_h(const PBITMAP bitmap);
int _bitmap_depth(const PBITMAP bitmap);
int _bitmap_pitch(const PBITMAP bitmap);
int _bitmap_pmo_cap(const PBITMAP bitmap);
u8 *_bitmap_raw_image(const PBITMAP bitmap);
RGB565 *_bitmap_ptr_rgb565(const PBITMAP bitmap);
RGB *_bitmap_ptr_rgb(const PBITMAP bitmap);
RGBA *_bitmap_ptr_rgba(const PBITMAP bitmap);

int do_bitblt(const PDC dst, int x1, int y1, const PDC src, int x2, int y2,
              int w, int h, enum BLT_OP bop, void *arg);
int do_stretch_blt(const PDC dst, int x1, int y1, int w1, int h1, const PDC src,
                   int x2, int y2, int w2, int h2, enum BLT_OP bop, void *arg);
int _bitblt(const PDC dst, int x1, int y1, int w, int h, const PDC src, int x2,
            int y2, enum ROP rop);
int _stretch_blt(const PDC dst, int x1, int y1, int w1, int h1, const PDC src,
                 int x2, int y2, int w2, int h2, enum ROP rop);
int _stretch_bitmap(const PDC dst, int x1, int y1, int w1, int h1,
                    const PBITMAP bitmap, int x2, int y2, int w2, int h2,
                    enum ROP rop);
int _transparent_blt(const PDC dst, int x1, int y1, int w1, int h1,
                     const PDC src, int x2, int y2, int w2, int h2, u32 color);

/* DIB functions */
PBITMAP _create_dib_bitmap(const PDC dc, const PDIB dib);
