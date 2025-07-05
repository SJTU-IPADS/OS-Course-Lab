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

#include <chcore/container/list.h>
#include <chcore-graphic/gui_defs.h>
#include <wayland-server-protocol.h>
#include <wayland-server-core.h>
#include <g2d/g2d.h>

enum WIN_STATE {
        WIN_UNINIT,
        WIN_INIT,
        WIN_MOVED,
        WIN_PAINTED,
        WIN_REMOVED,
        WIN_HIDDEN,
};
struct wm_window;
struct wm_window_type {
        enum { WIN_TYPE_TOPLEVEL,
               WIN_TYPE_FULLSCREEN,
               WIN_TYPE_TRANSIENT,
        } type;
        union {
                enum wl_shell_surface_fullscreen_method full_screen_flags;
                struct {
                        enum wl_shell_surface_transient transient_flag;
                        int x;
                        int y;
                        struct wm_window *parent;
                } transient_flags;
        } type_flags;
};

struct wm_window { /*Properties*/
        char name[10];

        /* Graphic */
        PBITMAP bitmap;
        PDC dc;
        PBITMAP mask; // Only for the contioner of a cursor surface.
        PDC dc_mask; // Only for the contioner of a cursor surface.
        PDC buffer_attached_dc;
        struct wl_resource *buffer_attached;
        struct list_head znode;
        PRGN damage;
        PRGN rgn_upd;
        PRGN rgn_upd_tmp;
        RECT rc;
        RECT *normal_rc;
        RECT new_rc;
        bool maximized;
        enum WIN_STATE state;

        /* Client */
        enum GUI_CLIENT_TYPE client_type;
        int win_id;
        struct ch_client *cwc; // for ChCore backend
        struct surface *surf; // for Wayland backend

        /* Window tree*/
        struct wm_window *parent;
        struct list_head pnode;
        struct list_head son_list;

        /*type*/
        struct wm_window_type type;
        struct wm_window_type type_tmp;

        /*scale*/
        double scale;
        double scale_tmp;
};


void wm_window_init_rgn(void);
struct wm_window *create_wm_window(void);
void del_wm_window(struct wm_window *win);
bool pt_in_wm_win(struct wm_window *win, int x, int y);
void wm_set_screen_size(int width, int height);

void wm_window_resize(struct wm_window *win, int new_l, int new_t, int new_r,
                      int new_b);
void wm_window_maximize(struct wm_window *win);
void wm_window_unmaximize(struct wm_window *win);
void wm_root_window_move(struct wm_window *win, int dx, int dy);
void wm_window_move(struct wm_window *win, int dx, int dy);
void wm_window_set_bitmap(struct wm_window *win, PBITMAP bitmap);
void wm_window_set_attached_bitmap(struct wm_window *win, PBITMAP bitmap);
void wm_window_set_cursor_mask(struct wm_window *win, PBITMAP mask);
PBITMAP wm_window_get_bitmap(struct wm_window *win);
void wm_window_damage(struct wm_window *win, int l, int t, int r, int b);
void wm_window_paint(struct wm_window *win);
