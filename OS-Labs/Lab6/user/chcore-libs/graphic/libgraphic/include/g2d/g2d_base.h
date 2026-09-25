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
 * Graphic base header file.
 * Define basic graphic types, structs and functions.
 */
#pragma once

#include <chcore/type.h>

#include "color_defs.h"

/* color_t is a u32 */
typedef u32 color_t; // COLORREF is a u32

/*
 * In BGR order:
 * b : 5
 * g : 6
 * r : 5
 */
typedef u16 RGB565;

/* In BGR order */
typedef struct {
        u8 b; // Blue
        u8 g; // Green
        u8 r; // Red
} RGB;

/* In BGRA order */
typedef union {
        struct {
                u8 b; // Blue
                u8 g; // Green
                u8 r; // Red
                u8 a; // Alpha
        };
        RGB rgb; // RGB triple (1st 3 bytes)
        color_t ref; // Color reference
} RGBA;

typedef struct rect {
        int l; // x-coord of left border
        int t; // y-coord of top border
        int r; // x-coord of right border
        int b; // y-coord of bottom border
} RECT;

typedef struct point {
        int x;
        int y;
} POINT;

/*
 * Color functions
 */

u8 get_rgb565_blue(RGB565 rgb565);
u8 get_rgb565_green(RGB565 rgb565);
u8 get_rgb565_red(RGB565 rgb565);
RGB565 rgb565_from_rgb(RGB color);
RGB rgb_from_rgb565(RGB565 rgb565);
RGBA rgba_from_rgb565(RGB565 rgb565);
RGBA rgba_from_rgb565_alpha(RGB565 rgb565, u8 alpha);
RGBA rgba_from_u32(u32 rgb);
RGBA rgba_from_u32_alpha(u32 rgb, u8 alpha);
RGBA rgba_from_rgb(RGB rgb);
RGBA rgba_from_rgb_alpha(RGB rgb, u8 alpha);

/*
 * Point functions
 */

POINT *create_point(int x, int y);
void delete_point(POINT *point);
void point_copy(POINT *dst, const POINT *src);

/*
 * Rectangle functions
 */

RECT *create_rect(int left, int top, int right, int bottom);
int set_rect(RECT *rc, int left, int top, int right, int bottom);
void delete_rect(RECT *rc);

int rect_width(const RECT *rc);
int rect_height(const RECT *rc);
bool rect_valid(const RECT *rc);
bool rect_valid2(int left, int top, int right, int bottom);

bool rect_equal(const RECT *rc1, const RECT *rc2);
void rect_move(RECT *rc, int x, int y);

bool pt_in_rect(const RECT *rc, int x, int y);
void print_rect(const RECT *rc);

u32 rect_and(RECT *dst, const RECT *src1, const RECT *src2);
u32 rect_diff(RECT dst[], const RECT *src, const RECT *clip);
