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

#include "wm_window.h"

#include <malloc.h>
#include "guilog/guilog.h"

static PRGN rgn_wm_win = NULL;
static int screen_width = 0;
static int screen_height = 0;

void wm_set_screen_size(int width, int height)
{
        screen_height = height;
        screen_width = width;
}

inline void wm_window_init_rgn(void)
{
        rgn_wm_win = create_region(32);
}

/* Return created wm_window */
struct wm_window *create_wm_window(void)
{
        struct wm_window *win = (struct wm_window *)malloc(sizeof(*win));

        win->bitmap = create_bitmap(0, 0, 0, 0);
        win->dc = create_dc(DC_OFFSCREEN);
        win->dc_mask = create_dc(DC_OFFSCREEN);
        win->buffer_attached_dc = create_dc(DC_OFFSCREEN);
        init_list_head(&win->znode);
        init_list_head(&win->pnode);
        init_list_head(&win->son_list);
        win->rgn_upd = create_region(8);
        win->rgn_upd_tmp = create_region(8);
        set_rect(&win->rc, 0, 0, 0, 0);
        set_rect(&win->new_rc, 0, 0, 0, 0);
        // set_rect(&win->normal_rc, 0, 0, 0, 0);
        win->maximized = 0;
        win->state = WIN_UNINIT;
        win->type.type = WIN_TYPE_TOPLEVEL;
        win->normal_rc = NULL;
        win->client_type = GUI_CLIENT_CHCORE;
        win->win_id = -1;
        win->cwc = NULL;
        win->surf = NULL;
        win->scale = 1;
        return win;
}

void del_wm_window(struct wm_window *win)
{
        delete_dc(win->dc);
        delete_region(win->rgn_upd);
        delete_region(win->rgn_upd_tmp);
        free(win);
}

bool pt_in_wm_win(struct wm_window *win, int x, int y)
{
        return x >= win->new_rc.l && x <= win->new_rc.r && y >= win->new_rc.t
               && y <= win->new_rc.b;
}

void wm_window_resize(struct wm_window *win, int new_l, int new_t, int new_r,
                      int new_b)
{
        win->new_rc.l = new_l;
        win->new_rc.t = new_t;
        win->new_rc.r = new_r;
        win->new_rc.b = new_b;
        if (win->new_rc.b == win->rc.b && win->new_rc.t == win->rc.t
            && win->new_rc.l == win->rc.l && win->new_rc.r == win->rc.r) {
                return;
        }
        GUI_DEBUG("resize: from ");
        GUI_DEBUG_PRINT_RECT(&win->rc);
        GUI_DEBUG("to: ");
        GUI_DEBUG_PRINT_RECT(&win->new_rc);
        int width = win->new_rc.r - win->new_rc.l + 1;
        int height = win->new_rc.b - win->new_rc.t + 1;
        if (win->bitmap) {
                delete_bitmap(win->bitmap);
        }
        win->bitmap = create_bitmap(width, height, 32, width * 4);
        dc_select_bitmap(win->dc, win->bitmap);
        // win->state = WIN_MOVED;
}

void wm_window_maximize(struct wm_window *win)
{
        // TODO： maximize window, useful in xdg_shell
}

void wm_window_unmaximize(struct wm_window *win)
{
        // TODO： unmaximize window, useful in xdg_shell
}

void wm_root_window_move(struct wm_window *win, int dx, int dy)
{
        if (win->parent != NULL) {
                return;
        }
        struct wm_window *son;
        for_each_in_list (son, struct wm_window, pnode, &win->son_list) {
                wm_window_move(son, dx, dy);
        }
        wm_window_move(win, dx, dy);
}

void wm_window_move(struct wm_window *win, int dx, int dy)
{
        win->new_rc.l += dx;
        win->new_rc.t += dy;
        win->new_rc.r += dx;
        win->new_rc.b += dy;
        if (win->state != WIN_UNINIT)
                win->state = WIN_MOVED;
}

void wm_window_set_attached_bitmap(struct wm_window *win, PBITMAP bitmap)
{
        dc_select_bitmap(win->buffer_attached_dc, bitmap);
}

/* Set bitmap directly, usually for initalizatio. */
void wm_window_set_bitmap(struct wm_window *win, PBITMAP bitmap)
{
        if (bitmap != NULL && win->state == WIN_UNINIT)
                win->state = WIN_INIT;
        win->bitmap = bitmap;
        dc_select_bitmap(win->dc, bitmap);
}

PBITMAP wm_window_get_bitmap(struct wm_window *win)
{
        return win->bitmap;
}

void wm_window_damage(struct wm_window *win, int l, int t, int r, int b)
{
        // if (win->state == WIN_INIT)
        // 	win->state = WIN_PAINTED;
        // if (win->state == WIN_PAINTED) {
        set_rect_region(rgn_wm_win, l, t, r, b);
        combine_region(win->rgn_upd_tmp, win->rgn_upd_tmp, rgn_wm_win, RGN_OR);
        // }
}

void wm_window_paint(struct wm_window *win)
{
        if ((win->new_rc.r - win->new_rc.l + 1
                     != get_dc_width(win->buffer_attached_dc)
             || win->new_rc.b - win->new_rc.t + 1
                        != get_dc_height(win->buffer_attached_dc))) {
                wm_window_resize(
                        win,
                        win->new_rc.l,
                        win->new_rc.t,
                        win->new_rc.l + get_dc_width(win->buffer_attached_dc)
                                - 1,
                        win->new_rc.t + get_dc_height(win->buffer_attached_dc)
                                - 1);
                set_rect_region(win->rgn_upd_tmp,
                                0,
                                0,
                                get_dc_width(win->buffer_attached_dc) - 1,
                                get_dc_height(win->buffer_attached_dc) - 1);
                win->state = WIN_MOVED;
        }
        if (region_null(win->rgn_upd_tmp)) {
                GUI_DEBUG("region null\n");
                return;
        }
        dc_release_clip_rgn(win->dc);
        dc_select_clip_rgn(win->dc, win->rgn_upd_tmp);
        bitblt(win->dc,
               0,
               0,
               win->new_rc.r - win->new_rc.l + 1,
               win->new_rc.b - win->new_rc.t + 1,
               win->buffer_attached_dc,
               0,
               0,
               ROP_SRCCOPY);
        combine_region(win->rgn_upd, win->rgn_upd, win->rgn_upd_tmp, RGN_OR);
        clear_region(win->rgn_upd_tmp);
        if (win->state == WIN_INIT || win->state == WIN_UNINIT) {
                win->state = WIN_PAINTED;
        }
}
