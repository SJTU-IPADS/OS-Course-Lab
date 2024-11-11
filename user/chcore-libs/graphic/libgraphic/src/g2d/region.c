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
 * Implementation of region functions.
 */
#include "region.h"

#include "dc.h"
#include "g2d_memop.h"
#include "../dep/os.h"

#define DEFAULT_RGN_SIZE 32

struct region {
        u32 size;
        u32 rc_num;
        RECT *rcs;
        u32 i;
};

static PRGN rgn_d0 = NULL;
static PRGN rgn_d1 = NULL;
static PRGN rgn_c0 = NULL;
static PRGN rgn_c1 = NULL;
static PRGN rgn_x0 = NULL;
static PRGN rgn_x1 = NULL;

static inline void init_gdi_rgn_diff(void)
{
        if (rgn_d0 == NULL)
                rgn_d0 = _create_rgn(DEFAULT_RGN_SIZE);
        else
                _clear_rgn(rgn_d0);
        if (rgn_d1 == NULL)
                rgn_d1 = _create_rgn(DEFAULT_RGN_SIZE);
        else
                _clear_rgn(rgn_d1);
}

static inline void init_gdi_rgn_comb(void)
{
        if (rgn_c0 == NULL)
                rgn_c0 = _create_rgn(DEFAULT_RGN_SIZE);
        else
                _clear_rgn(rgn_c0);
        if (rgn_c1 == NULL)
                rgn_c1 = _create_rgn(DEFAULT_RGN_SIZE);
        else
                _clear_rgn(rgn_c1);
}

static u32 get_expand_size(u32 old_size, u32 new_num)
{
        u32 new_size = old_size;
        while (new_size < new_num)
                new_size <<= 1;
        return new_size;
}

static void expand_rgn(PRGN rgn)
{
        RECT *new_rcs;

        rgn->size <<= 1;
        new_rcs = (RECT *)malloc(sizeof(RECT) * rgn->size);
        memcpy(new_rcs, rgn->rcs, sizeof(RECT) * rgn->rc_num);
        free(rgn->rcs);
        rgn->rcs = new_rcs;
}

static void rgn_copy(PRGN dst, const PRGN src)
{
        if (dst == src)
                return;

        dst->rc_num = src->rc_num;
        if (_rgn_null(src))
                return;
        if (dst->size >= src->rc_num)
                memcpy(dst->rcs, src->rcs, sizeof(RECT) * src->rc_num);
        else {
                dst->size = get_expand_size(dst->size, src->rc_num);
                dst->rcs = (RECT *)realloc(dst->rcs, sizeof(RECT) * dst->size);
                memcpy(dst->rcs, src->rcs, sizeof(RECT) * src->rc_num);
        }
}

static void rgn_push_rgn(PRGN dst, const PRGN src)
{
        RECT *new_rcs;
        if (_rgn_null(src))
                return;
        if (dst->rc_num + src->rc_num > dst->size) {
                dst->size =
                        get_expand_size(dst->size, dst->rc_num + src->rc_num);
                new_rcs = (RECT *)malloc(sizeof(RECT) * dst->size);
                memcpy(new_rcs, dst->rcs, sizeof(RECT) * dst->rc_num);
                free(dst->rcs);
                dst->rcs = new_rcs;
        }
        memcpy(dst->rcs + dst->rc_num, src->rcs, sizeof(RECT) * src->rc_num);
        dst->rc_num += src->rc_num;
}

static void rgn_push_rc(PRGN rgn, const RECT *rc)
{
        if (rgn->rc_num >= rgn->size)
                expand_rgn(rgn);
        rgn->rcs[rgn->rc_num] = *rc;
        ++rgn->rc_num;
}

static inline int combine_rgn_add(PRGN dst, const PRGN src)
{
        rgn_push_rgn(dst, src);
        return 0;
}

static int combine_rgn_and(PRGN dst, PRGN src1, PRGN src2)
{
        u32 i, j;
        RECT rc;
        if (_rgn_null(src1) || _rgn_null(src2)){
                clear_region(dst);
                return 0;
        }
        init_gdi_rgn_diff();

        for (i = 0; i < src1->rc_num; ++i) {
                for (j = 0; j < src2->rc_num; ++j) {
                        if (rect_and(&rc, src1->rcs + i, src2->rcs + j) == 1)
                                rgn_push_rc(rgn_d0, &rc);
                }
        }
        clear_region(dst);
        rgn_push_rgn(dst, rgn_d0);
        return 0;
}

static inline int combine_rgn_copy(PRGN dst, const PRGN src)
{
        rgn_copy(dst, src);
        return 0;
}

static int combine_rgn_diff(PRGN dst, PRGN src, PRGN clip)
{
        u32 i, j;
        RECT rcs[4];
        PRGN r0, r1, rt;
        u32 rc_num, k;

        if (_rgn_null(src)) {
                _clear_rgn(dst);
                return 0;
        }
        if (_rgn_null(clip)) {
                rgn_copy(dst, src);
                return 0;
        }

        init_gdi_rgn_diff();
        r0 = rgn_d0;
        r1 = rgn_d1;

        /* Clip src by clip */
        rgn_push_rgn(r1, src);
        for (i = 0; i < clip->rc_num; ++i) {
                /* Always operate on r1 and push results to r0 */
                for (j = 0; j < r1->rc_num; ++j) {
                        rc_num = rect_diff(rcs, r1->rcs + j, clip->rcs + i);
                        for (k = 0; k < rc_num; ++k)
                                rgn_push_rc(r0, rcs + k);
                }
                /* Swap r0 and r1, then clear r0 */
                rt = r0;
                r0 = r1;
                r1 = rt;
                _clear_rgn(r0);
        }

        /* Flush results to dst */
        _clear_rgn(dst);
        rgn_push_rgn(dst, r1);
        return 0;
}

static int combine_rgn_or(PRGN dst, PRGN src1, PRGN src2)
{
        if (dst == src2) {
                if (dst == src1)
                        return 0;
                init_gdi_rgn_comb();
                // Reserve src2
                rgn_push_rgn(rgn_c0, src2);
                // dst = src1 - src2
                combine_rgn_diff(dst, src1, src2);
                // dst += src2
                rgn_push_rgn(dst, rgn_c0);
        } else {
                // dst = src1 - src2
                combine_rgn_diff(dst, src1, src2);
                // dst += src2
                rgn_push_rgn(dst, src2);
        }
        return 0;
}

static int combine_rgn_xor(PRGN dst, PRGN src1, PRGN src2)
{
        if (dst == src1 && dst == src2) {
                _clear_rgn(dst);
                return 0;
        }
        init_gdi_rgn_comb();
        // rgn_c0 = src1 - src2
        combine_rgn_diff(rgn_c0, src1, src2);
        // rgn_c1 = src2 - src1
        combine_rgn_diff(rgn_c1, src1, src2);
        // dst = rgn_c0 + rgn_c1
        _clear_rgn(dst);
        rgn_push_rgn(dst, rgn_c0);
        rgn_push_rgn(dst, rgn_c1);
        return 0;
}

PRGN _create_rgn(int size)
{
        PRGN rgn = (PRGN)malloc(sizeof(*rgn));
        rgn->size = size > 0 ? size : 1;
        rgn->rc_num = 0;
        rgn->rcs = (RECT *)malloc(sizeof(RECT) * rgn->size);
        rgn->i = 0;
        return rgn;
}

int _set_rc_rgn(PRGN rgn, int l, int t, int r, int b)
{
        if (!rect_valid2(l, t, r, b)) {
                rgn->rc_num = 0;
                return -1;
        } else {
                rgn->rc_num = 1;
                rgn->i = 0;
                set_rect(rgn->rcs, l, t, r, b);
                return 0;
        }
}

inline int _set_dc_rgn(PRGN rgn, const PDC dc)
{
        return _set_rc_rgn(rgn, 0, 0, dc_width(dc) - 1, dc_height(dc) - 1);
}

void _del_rgn(PRGN rgn)
{
        if (rgn->rcs != NULL)
                free(rgn->rcs);
        free(rgn);
}

RECT *_get_rgn_rc(PRGN rgn, enum GET_RGN_RC_CMD cmd)
{
        if (_rgn_null(rgn))
                return NULL;
        switch (cmd) {
        case GRR_FIRST:
                rgn->i = 0;
                return rgn->rcs;
        case GRR_LAST:
                rgn->i = rgn->rc_num - 1;
                return rgn->rcs + rgn->i;
        case GRR_NEXT:
                if (rgn->i >= rgn->rc_num)
                        return NULL;
                else {
                        ++rgn->i;
                        if (rgn->i >= rgn->rc_num)
                                return NULL;
                        return rgn->rcs + rgn->i;
                }
        case GRR_PREV:
                if (rgn->i <= -1)
                        return NULL;
                else {
                        --rgn->i;
                        if (rgn->i <= -1)
                                return NULL;
                        return rgn->rcs + rgn->i;
                }
        default:
                return NULL;
        }
}

int _combine_rgn(PRGN dst, PRGN src1, PRGN src2, enum RGN_COMB_MODE mode)
{
        dst->i = 0;
        switch (mode) {
        case RGN_ADD:
                return combine_rgn_add(dst, src1);
        case RGN_AND:
                if (src2 == NULL)
                        return -1;
                return combine_rgn_and(dst, src1, src2);
        case RGN_COPY:
                return combine_rgn_copy(dst, src1);
        case RGN_DIFF:
                if (src2 == NULL)
                        return -1;
                return combine_rgn_diff(dst, src1, src2);
        case RGN_OR:
                if (src2 == NULL)
                        return -1;
                return combine_rgn_or(dst, src1, src2);
        case RGN_XOR:
                if (src2 == NULL)
                        return -1;
                return combine_rgn_xor(dst, src1, src2);
        default:
                return -1;
        }
        return -1;
}

inline void _clear_rgn(PRGN rgn)
{
        rgn->rc_num = 0;
        rgn->i = 0;
}

void _move_rgn(PRGN rgn, int x, int y)
{
        u32 i;

        if (rgn->rcs == NULL)
                return;
        for (i = 0; i < rgn->rc_num; ++i)
                rect_move(rgn->rcs + i, x, y);
}

bool _pt_in_rgn(const PRGN rgn, int x, int y)
{
        u32 i;

        if (rgn->rcs == NULL)
                return false;
        for (i = 0; i < rgn->rc_num; ++i) {
                if (pt_in_rect(rgn->rcs + i, x, y))
                        return true;
        }
        return false;
}

inline bool _rgn_null(const PRGN rgn)
{
        return rgn->rc_num == 0;
}

/* For debug */
void _print_rgn(const PRGN rgn)
{
        RECT *rc;
        u32 i;

        if (rgn->rcs == NULL || rgn->rc_num == 0) {
                printf("[NULL rgn]\n");
                return;
        }
        for (i = 0; i < rgn->rc_num; ++i) {
                rc = rgn->rcs + i;
                printf("[%d %d %d %d]", rc->l, rc->t, rc->r, rc->b);
        }
        printf("\n");
}

inline u32 rgn_rc_num(const PRGN rgn)
{
        return rgn->rc_num;
}

inline const RECT *rgn_rc(const PRGN rgn, u32 i)
{
        if (i >= rgn->rc_num)
                return NULL;
        return rgn->rcs + i;
}

inline void init_gdi_rgn_ex(void)
{
        if (rgn_x0 == NULL)
                rgn_x0 = _create_rgn(DEFAULT_RGN_SIZE);
        else
                _clear_rgn(rgn_x0);
        if (rgn_x1 == NULL)
                rgn_x1 = _create_rgn(DEFAULT_RGN_SIZE);
        else
                _clear_rgn(rgn_x1);
}

inline PRGN get_gdi_rgn_ex0(void)
{
        return rgn_x0;
}

inline PRGN get_gdi_rgn_ex1(void)
{
        return rgn_x1;
}
