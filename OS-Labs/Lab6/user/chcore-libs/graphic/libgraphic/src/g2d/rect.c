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
 * Implementation of rectangle functions.
 */
#include "g2d_internal.h"

#include "g2d_math.h"
#include "../dep/os.h"

/*
 * Rectangle clip macros.
 * Clip outer rect by inner rect into 4 rects:
 *
 * -------------------
 * |        T        |
 * -------------------
 * | L |   clip  | R |
 * -------------------
 * |        B        |
 * -------------------
 *
 * Split y-coord first.
 */

#define RECT_CLIP_LEFT(dst, src, clip)                   \
        {                                                \
                (dst)->l = (src)->l;                     \
                (dst)->t = MAX((clip)->t, (src)->t);     \
                (dst)->r = MIN((clip)->l - 1, (src)->r); \
                (dst)->b = MIN((clip)->b, (src)->b);     \
        }
#define RECT_CLIP_TOP(dst, out, clip)                    \
        {                                                \
                (dst)->l = (src)->l;                     \
                (dst)->t = (src)->t;                     \
                (dst)->r = (src)->r;                     \
                (dst)->b = MIN((clip)->t - 1, (src)->b); \
        }
#define RECT_CLIP_RIGHT(dst, src, clip)                \
        {                                              \
                (dst)->l = MAX((clip)->r + 1, src->l); \
                (dst)->t = MAX((clip)->t, (src)->t);   \
                (dst)->r = (src)->r;                   \
                (dst)->b = MIN((clip)->b, (src)->b);   \
        }
#define RECT_CLIP_BOTTOM(dst, src, clip)                 \
        {                                                \
                (dst)->l = (src)->l;                     \
                (dst)->t = MAX((clip)->b + 1, (src)->t); \
                (dst)->r = (src)->r;                     \
                (dst)->b = (src)->b;                     \
        }

/*
 * Rectangle property macros
 */

#define RECT_VALID(rc) ((rc)->l <= (rc)->r && (rc)->t <= (rc)->b)

RECT *create_rect(int l, int t, int r, int b)
{
        RECT *rc = (RECT *)malloc(sizeof(*rc));
        rc->l = l;
        rc->t = t;
        rc->r = r;
        rc->b = b;
        return rc;
}

int set_rect(RECT *rc, int l, int t, int r, int b)
{
        if (rc == NULL)
                return -1;
        rc->l = l;
        rc->t = t;
        rc->r = r;
        rc->b = b;
        return 0;
}

void delete_rect(RECT *rc)
{
        if (rc != NULL)
                free(rc);
}

/*
 * Return width of the rectangle.
 */
inline int rect_width(const RECT *rc)
{
        return rc->r - rc->l + 1;
}

/*
 * Return height of the rectangle.
 */
inline int rect_height(const RECT *rc)
{
        return rc->b - rc->t + 1;
}

/*
 * Return if the rectangle is valid.
 */
inline bool rect_valid(const RECT *rc)
{
        return RECT_VALID(rc);
}

/*
 * Return if the rectangle formed by the args is valid.
 */
inline bool rect_valid2(int l, int t, int r, int b)
{
        return l <= r && t <= b;
}

/*
 * Return if the two rectanles are totally same.
 */
inline bool rect_equal(const RECT *rc1, const RECT *rc2)
{
        return rc1->l == rc2->l && rc1->t == rc2->t && rc1->r == rc2->r
               && rc1->b == rc2->b;
}

/*
 * Move the rectangle by adding (x, y).
 */
inline void rect_move(RECT *rc, int x, int y)
{
        rc->l += x;
        rc->t += y;
        rc->r += x;
        rc->b += y;
}

/*
 * Return if point (x, y) locates in the rectangle.
 */
inline bool pt_in_rect(const RECT *rc, int x, int y)
{
        return x >= rc->l && x <= rc->r && y >= rc->t && y <= rc->b;
}

/*
 * Print info of the rectangle.
 * Just for debug.
 */
void print_rect(const RECT *rc)
{
        if (rc == NULL) {
                printf("[NULL rect]\n");
                return;
        }
        printf("[%d %d %d %d]\n", rc->l, rc->t, rc->r, rc->b);
}

/*
 * Logic AND of two rectangles.
 * Set dst rect as the intersect rect of two src rects.
 * Return num of dst rects. (in this case, 0 or 1)
 */
u32 rect_and(RECT *dst, const RECT *src1, const RECT *src2)
{
        dst->l = MAX(src1->l, src2->l);
        dst->t = MAX(src1->t, src2->t);
        dst->r = MIN(src1->r, src2->r);
        dst->b = MIN(src1->b, src2->b);
        if (RECT_VALID(dst))
                return 1;
        return 0;
}

/*
 * Logic DIFF of two rectangles.
 * Set dst rects as the result of src1 MINUS src2.
 * We make sure that there's no overlap in dst rects.
 * Return num of dst rects. (in this case, 0 ~ 4)
 */
u32 rect_diff(RECT dst[], const RECT *src, const RECT *clip)
{
        u32 i = 0;
        RECT_CLIP_LEFT(dst + i, src, clip);
        if (RECT_VALID(dst + i))
                ++i;
        RECT_CLIP_TOP(dst + i, src, clip);
        if (RECT_VALID(dst + i))
                ++i;
        RECT_CLIP_RIGHT(dst + i, src, clip);
        if (RECT_VALID(dst + i))
                ++i;
        RECT_CLIP_BOTTOM(dst + i, src, clip);
        if (RECT_VALID(dst + i))
                ++i;
        return i;
}
