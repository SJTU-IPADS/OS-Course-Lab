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
 * Implementation of line functions.
 */
#include "line.h"

#include "dc.h"
#include "g2d_math.h"
#include "region.h"

/* Return -1 if line out of rect */
static int clip_hline_with_rect(const RECT *rc, int l, int r, int y, int *cl,
                                int *cr)
{
        if (y < rc->t || y > rc->b || l > rc->r || r < rc->l)
                return -1;
        *cl = MAX(rc->l, l);
        *cr = MIN(rc->r, r);
        return 0;
}

/* Return -1 if line out of rect */
static int clip_vline_with_rect(const RECT *rc, int x, int t, int b, int *ct,
                                int *cb)
{
        if (x < rc->l || x > rc->r || t > rc->b || b < rc->t)
                return -1;
        *ct = MAX(rc->t, t);
        *cb = MIN(rc->b, b);
        return 0;
}

/*
 * Return -1 if line out of rect.
 * Liang-Barsky algorithm.
 */
static int clip_line_with_rect(const RECT *rc, int x1, int y1, int x2, int y2,
                               int *cx1, int *cy1, int *cx2, int *cy2)
{
        int p[4], q[4];
        u32 i;
        float u1 = 0, u2 = 1, u;

        p[0] = x1 - x2;
        p[1] = x2 - x1;
        p[2] = y1 - y2;
        p[3] = y2 - y1;
        q[0] = x1 - rc->l;
        q[1] = rc->r - x1;
        q[2] = y1 - rc->t;
        q[3] = rc->b - y1;

        for (i = 0; i < 4; ++i) {
                /* Line parallel out of rect */
                if (p[i] == 0 && q[i] < 0)
                        return -1;
                /* Otherwise */
                else if (p[i] != 0) {
                        u = q[i] / p[i];
                        if (p[i] < 0) {
                                u1 = MAX(u1, u);
                        } else {
                                u2 = MIN(u2, u);
                        }
                }
        }

        if (u1 > u2)
                return -1;
        *cx1 = x1 + u1 * (x2 - x1);
        *cy1 = y1 + u1 * (y2 - y1);
        *cx2 = x1 + u2 * (x2 - x1);
        *cy2 = y1 + u2 * (y2 - y1);
        return 0;
}

int _draw_hline(const PDC dc, int l, int r, int y)
{
        int cleft0, cright0, cl, cr;
        u32 i;
        RECT rc;

        set_rect(&rc, 0, 0, dc_width(dc) - 1, dc_height(dc) - 1);
        if (clip_hline_with_rect(&rc, l, r, y, &cleft0, &cright0) != 0)
                return -1;

        if (dc_clip_rgn(dc) == NULL)
                dc_gfx(dc)->draw_hline(dc, cleft0, cright0, y);
        else {
                for (i = 0; i < rgn_rc_num(dc_clip_rgn(dc)); ++i) {
                        if (clip_hline_with_rect(rgn_rc(dc_clip_rgn(dc), i),
                                                 cleft0,
                                                 cright0,
                                                 y,
                                                 &cl,
                                                 &cr)
                            == 0)
                                dc_gfx(dc)->draw_hline(dc, cl, cr, y);
                }
        }
        return 0;
}

int _draw_vline(const PDC dc, int x, int t, int b)
{
        int ct0, cb0, ct, cb;
        u32 i;
        RECT rc;

        set_rect(&rc, 0, 0, dc_width(dc) - 1, dc_height(dc) - 1);
        if (clip_vline_with_rect(&rc, x, t, b, &ct0, &cb0) != 0)
                return -1;

        if (dc_clip_rgn(dc) == NULL)
                dc_gfx(dc)->draw_vline(dc, x, ct0, cb0);
        else {
                for (i = 0; i < rgn_rc_num(dc_clip_rgn(dc)); ++i) {
                        if (clip_vline_with_rect(rgn_rc(dc_clip_rgn(dc), i),
                                                 x,
                                                 ct0,
                                                 cb0,
                                                 &ct,
                                                 &cb)
                            == 0)
                                dc_gfx(dc)->draw_vline(dc, x, ct, cb);
                }
        }
        return 0;
}

int _draw_line(const PDC dc, int x1, int y1, int x2, int y2)
{
        int min, max;
        u32 i;
        RECT rc;
        int cx1, cy1, cx2, cy2;
        int cx3, cy3, cx4, cy4;

        if (x1 == x2) {
                min = MIN(y1, y2);
                max = MAX(y1, y2);
                return _draw_vline(dc, x1, min, max);
        } else if (y1 == y2) {
                min = MIN(x1, x2);
                max = MAX(x1, x2);
                return _draw_hline(dc, min, max, y1);
        }

        set_rect(&rc, 0, 0, dc_width(dc) - 1, dc_height(dc) - 1);
        if (clip_line_with_rect(&rc, x1, y1, x2, y2, &cx1, &cy1, &cx2, &cy2)
            != 0)
                return -1;

        if (dc_clip_rgn(dc) == NULL)
                dc_gfx(dc)->draw_line(dc, cx1, cy1, cx2, cy2);
        else {
                for (i = 0; i < rgn_rc_num(dc_clip_rgn(dc)); ++i) {
                        if (clip_line_with_rect(rgn_rc(dc_clip_rgn(dc), i),
                                                cx1,
                                                cy1,
                                                cx2,
                                                cy2,
                                                &cx3,
                                                &cy3,
                                                &cx4,
                                                &cy4)
                            == 0)
                                dc_gfx(dc)->draw_line(dc, cx3, cy3, cx4, cy4);
                }
        }
        return 0;
}

int _draw_rect(const PDC dc, int l, int t, int r, int b)
{
        _draw_hline(dc, l, r, t);
        _draw_hline(dc, l, r, b);
        _draw_vline(dc, l, t, b);
        _draw_vline(dc, r, t, b);
        return 0;
}

int _fill_rect(const PDC dc, int l, int t, int r, int b)
{
        PRGN rgn0, rgn1;
        const RECT *rc;
        u32 i, j;
        j=2048;
        
        init_gdi_rgn_ex();
        
        rgn0 = get_gdi_rgn_ex0();
        
        rgn1 = get_gdi_rgn_ex1();
        
        /* Init rgn0 by target rect */
        _set_rc_rgn(rgn0, l, t, r, b);
        
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
        
        /* Fill rects of rgn0 */
        for (i = 0; i < rgn_rc_num(rgn0); ++i) {
                rc = rgn_rc(rgn0, i);
                dc_gfx(dc)->fill_rect(dc, rc->l, rc->t, rc->r, rc->b);
        }
        return 0;
}
