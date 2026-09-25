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

#pragma once

#include <g2d/g2d.h>


#define DEFAULT_CURSOR_WIDTH 48
#define DEFAULT_CURSOR_HEIGHT 48
/* Information of wm_cursor displayed on screen */
struct wm_cursor {
        int x;
        int y;
        int w; // width
        int h; // height
        int new_x;
        int new_y;

        int hotspot_x;
        int hotspot_y;

        bool changed;

        /* Display */
        PDC dc_cursor;
};

void set_cursor_to_default(struct wm_cursor *cs);

struct wm_cursor *create_wm_cursor(int x, int y, int w, int h);
void del_wm_cursor(struct wm_cursor *cs);

void wm_cursor_create_bitmap(struct wm_cursor *cs, const PDC dc);
void wm_cursor_create_bitmap_from_buffer(struct wm_cursor *cs, const PDC dc,
                                         PBITMAP bitmap, void *buffer);
void wm_cursor_paint(struct wm_cursor *cs, PDC dc);
void wm_cursor_move(struct wm_cursor *cs, int dx, int dy);
