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
#include "wm_cursor.h"
#include "wm_window.h"


void init_winmgr(void);
void wm_set_screen_bitmap(PBITMAP bitmap);
/*hint: buffer is the raw data of bitmap*/
void wm_set_cursor_bitmap(struct wm_window *win, void *buffer);
void wm_add_win(struct wm_window *win);
void wm_rm_win(struct wm_window *win);
void wm_win_change_parent(struct wm_window *parent, struct wm_window *son);
struct wm_window *wm_get_win_by_pos(int x, int y);
void wm_clear_top_win(void);
void wm_set_top_win(struct wm_window *win);

struct wm_cursor *wm_get_cursor(void);
// set a window as a cursor window, without display it as a normal one.
void wm_set_cursor_win(struct wm_window *win);
// set the cursor window active.
void wm_commit_cursor_win(struct wm_window *win);
void wm_set_cursor_to_default();
void wm_border_fix(int x, int y, int *dx, int *dy);
#define GET_SCREEN_W(hw) ((hw)&0xFFFF)
#define GET_SCREEN_H(hw) ((hw) >> 32)
uint64_t wm_get_screen_size(); // return wm.h << 32 | wm.w;
void wm_comp(void);

#define PATCH_COORD(x, y) ((((uint64_t)(x)) << 32) | ((uint64_t)(y)))
#define COORD_X(patch)    (((uint64_t)(patch)) >> 32)
#define COORD_Y(patch)    (((uint64_t)(patch)) & 0xFFFF)
inline uint64_t buffer_coord_to_win_coord(uint64_t coord, struct wm_window *win)
{
        return PATCH_COORD((COORD_X(coord) * win->scale),
                           (COORD_Y(coord) * win->scale));
}
inline uint64_t buffer_coord_to_win_coord_single(uint64_t x,
                                                 struct wm_window *win)
{
        return x * win->scale;
}
