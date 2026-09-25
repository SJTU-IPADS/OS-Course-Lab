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
 * Graphics function header file.
 */
#pragma once

#include "g2d_internal.h"

enum BLT_OP { BLT_RASTER, BLT_BLEND, BLT_BLEND_RGBA, BLT_TRANSPARENT };

/* Base graphic functions */
struct base_gfx {
        void (*draw_pixel)(const PDC dc, int x, int y);
        void (*draw_hline)(const PDC dc, int l, int r, int y);
        void (*draw_vline)(const PDC dc, int x, int t, int b);
        void (*draw_line)(const PDC dc, int x1, int y1, int x2, int y2);
        void (*fill_rect)(const PDC dc, int l, int t, int r, int b);
        void (*draw_arc)(const PDC dc, int x1, int y1, int x2, int y2, int x3,
                         int y3, int x4, int y4);
        void (*draw_ellipse)(const PDC dc, int l, int t, int r, int b);
        void (*draw_char)(const PDC dc, char ch, int x, int y, int l, int t,
                          int r, int b);
        void (*bitblt)(const PDC dst_dc, int x1, int y1, const PDC src_dc,
                       int x2, int y2, int w, int h, enum BLT_OP bop,
                       void *arg);
        void (*stretch_blt)(const PDC dst_dc, int x1, int y1, int sx1, int sy1,
                            const PDC src_dc, int x2, int y2, int sx2, int sy2,
                            int px, int py, int pw, int ph, enum BLT_OP bop,
                            void *arg);
};

struct base_gfx *get_base_gfx_16(void);
struct base_gfx *get_base_gfx_24(void);
struct base_gfx *get_base_gfx_32(void);
