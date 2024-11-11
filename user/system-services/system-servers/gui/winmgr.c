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

#include "winmgr.h"

#include <stdio.h> // debug
#include <chcore/container/list.h>
#include <malloc.h>

#include <g2d/g2d.h>
#include "wm_cursor.h"
#include "wm_window.h"
#include "wayland_backend.h"

#include "guilog/guilog.h"

#define WALLPAPER_FILE "/sys/gui/resources/wallpaper.bmp"

struct winmgr {
        struct wm_cursor *cs;
        struct wm_window *cs_win;
        /*The list of all windows*/
        struct list_head zlist;
        /*The list of top-level windows*/
        struct list_head plist;
        /* TODO: multi-output */
        PBITMAP screen_bitmap;

        RECT full_screen_rect;
        PDC dc;
        enum WIN_STATE screen_state;
        int w;
        int h;
        int depth;
        int pitch;
        struct wm_window* top_win;
};

static struct winmgr wm = { 0 };

// Following are the tmp variables for window composition and repaint.
static PRGN rgn_clip = NULL;    // The clip region, which is the union of window that are on the higher level.
static PRGN rgn_upd = NULL;     // The update region, which is the region that needs to be updated.
static PRGN rgn_win = NULL;     // The region just for calculation.
static PRGN rgn_win2 = NULL;    // The region just for calculation.

static void wm_exact_remove_win(struct wm_window *win)
{
        GUI_DEBUG("[WINMGR] Delete window %p\n", win);
        list_del(&win->znode);
        list_del(&win->pnode);
        del_wm_window(win);
        GUI_DEBUG("[WINMGR] Successfully deleted\n");
}

static void wm_process_update_window(struct wm_window *win)
{
        struct wm_window *son;
        if (!list_empty(&(win->son_list))) {
                for_each_in_list (
                        son, struct wm_window, pnode, &(win->son_list)) {
                        wm_process_update_window(son);
                }
        }
        if (win->state == WIN_UNINIT) {
                // printf("WIN_UNINIT\n"); // debug
                return;
        }
        switch (win->state) {
        /* rgn_clip |= win->rc */
        case WIN_INIT:
                // GUI_DEBUG("WIN_INIT: %p\n", win); // debug
                set_rc_region(rgn_win, &win->rc);
                combine_region(rgn_clip, rgn_clip, rgn_win, RGN_OR);
                break;
        /*
         * rgn_upd += (win->rc | win->new_rc) - rgn_clip
         * rgn_clip += (win->rc | win->new_rc) - rgn_clip
         */
        case WIN_MOVED:
                // Never a fullscreen window.
                // GUI_DEBUG("WIN_MOVED: %p\n", win); // debug
                set_rc_region(rgn_win, &win->rc);
                set_rc_region(rgn_win2, &win->new_rc);
                combine_region(rgn_win, rgn_win, rgn_win2, RGN_OR);
                combine_region(rgn_win, rgn_win, rgn_clip, RGN_DIFF);
                if (!region_null(rgn_win)) {
                        combine_region(rgn_upd, rgn_win, NULL, RGN_ADD);
                        combine_region(rgn_clip, rgn_win, NULL, RGN_ADD);
                }
                win->rc = win->new_rc;
                break;
        /*
         * rgn_upd += win->rgn_upd - rgn_clip
         * rgn_clip += win->rgn_upd - rgn_clip
         */
        case WIN_PAINTED:
                // GUI_DEBUG("WIN_PAINTED: %p\n", win); // debug
                // save_window(win->bitmap, win->rgn_upd);
                win->rc = win->new_rc;
                // GUI_DEBUG_PRINT_REGION(win->rgn_upd);
                move_region(win->rgn_upd, win->rc.l, win->rc.t);
                combine_region(rgn_win, win->rgn_upd, rgn_clip, RGN_DIFF);
                if (!region_null(rgn_win)) {
                        combine_region(rgn_upd, rgn_win, NULL, RGN_ADD);
                        combine_region(rgn_clip, rgn_win, NULL, RGN_ADD);
                }
                break;
        case WIN_HIDDEN:
                set_rc_region(rgn_win, &win->rc);
                combine_region(rgn_win, rgn_win, rgn_clip, RGN_DIFF);
                if (!region_null(rgn_win)) {
                        combine_region(rgn_upd, rgn_win, NULL, RGN_ADD);
                        combine_region(rgn_clip, rgn_win, NULL, RGN_ADD);
                }
                wm_rm_win(win);
                break;
        case WIN_REMOVED:
                set_rc_region(rgn_win, &win->rc);
                combine_region(rgn_win, rgn_win, rgn_clip, RGN_DIFF);
                if (!region_null(rgn_win)) {
                        combine_region(rgn_upd, rgn_win, NULL, RGN_ADD);
                        combine_region(rgn_clip, rgn_win, NULL, RGN_ADD);
                }
                wm_exact_remove_win(win);
                return;
        default:
                break;
        }
        clear_region(win->rgn_upd);
        win->state = WIN_INIT;
}

static void wm_process_updates(void)
{
        struct wm_window *win;
        struct wm_cursor *cs = wm.cs;

        // rgn_clip = 0, rgn_upd = 0
        clear_region(rgn_clip);
        clear_region(rgn_upd);

        /* Process window updates */
        for_each_in_list (win, struct wm_window, pnode, &wm.plist) {
                wm_process_update_window(win);
        }

        /* Process cursor updates */
        // If moved: rgn_upd |= rgn(cs.old)
        if (cs->x != cs->new_x || cs->y != cs->new_y || cs->changed == true) {
                set_rect_region(rgn_win,
                                cs->x,
                                cs->y,
                                cs->x + cs->w - 1,
                                cs->y + cs->h - 1);
                combine_region(rgn_upd, rgn_win, NULL, RGN_ADD);
                set_rect_region(rgn_win,
                                cs->new_x,
                                cs->new_y,
                                cs->new_x + cs->w - 1,
                                cs->new_y + cs->h - 1);
                combine_region(rgn_upd, rgn_win, NULL, RGN_ADD);
        }
}

static void wm_window_repaint(struct wm_window *win)
{
        struct wm_window *son;
        if (!list_empty(&(win->son_list))) {
                for_each_in_list (
                        son, struct wm_window, pnode, &(win->son_list)) {
                        wm_window_repaint(son);
                }
        }
        if (win->state == WIN_UNINIT)
                return;
        // rgn_win = win->rc
        set_rc_region(rgn_win, &win->rc);
        GUI_DEBUG("rgn_win = win->rc:"); // debug
        // rgn_win &= rgn_upd
        combine_region(rgn_win, rgn_win, rgn_upd, RGN_AND);
        if (region_null(rgn_win))
                return;
        GUI_DEBUG("rgn_win &= rgn_upd:"); // debug
        GUI_DEBUG_PRINT_REGION(rgn_win); // debug
        // rgn_win -= rgn_clip
        combine_region(rgn_win, rgn_win, rgn_clip, RGN_DIFF);
        if (region_null(rgn_win))
                return;
        GUI_DEBUG("rgn_win -= rgn_clip:"); // debug
        GUI_DEBUG_PRINT_REGION(rgn_win); // debug
        // bitblt rgn_win
        dc_select_clip_rgn(wm.dc, rgn_win);

        bitblt(wm.dc,
               win->rc.l,
               win->rc.t,
               win->rc.r - win->rc.l + 1,
               win->rc.b - win->rc.t + 1,
               win->dc,
               0,
               0,
               ROP_SRCCOPY);
        
        // rgn_clip += rgn_win
        combine_region(rgn_clip, rgn_win, NULL, RGN_ADD);

        GUI_DEBUG("rgn_clip += rgn_win:"); // debug
        GUI_DEBUG(rgn_clip); // debug
}

static void wm_repaint(void)
{
        PDC dc = wm.dc;
        struct wm_window *win;
        struct wm_cursor *cs = wm.cs;
        if (wm.screen_state == WIN_UNINIT)
                return;
        else if (wm.screen_state == WIN_MOVED) {
                GUI_DEBUG("[GUI] screen_state: WIN_MOVED\n"); // debug
                set_rect_region(rgn_upd, 0, 0, wm.w - 1, wm.h - 1);
                GUI_DEBUG_PRINT_REGION(rgn_upd); // debug
        }

        if (region_null(rgn_upd)) {
                GUI_DEBUG("rgn_upd NULL\n"); // debug
        }
        GUI_DEBUG("rgn_upd:"); // debug
        GUI_DEBUG_PRINT_REGION(rgn_upd); // debug

	// rgn_clip = 0
	clear_region(rgn_clip);

        /* Repaint window */
        for_each_in_list (win, struct wm_window, pnode, &wm.plist) {
                wm_window_repaint(win);
        }
        /* Repaint cursor */
        // If moved: repaint whole cs.new
        if (cs->x != cs->new_x || cs->y != cs->new_y || cs->changed == true) {
                cs->x = cs->new_x;
                cs->y = cs->new_y;
                dc_release_clip_rgn(dc);
                wm_cursor_paint(cs, dc);
                cs->changed = false;
        }
        // If NOT moved: repaint (rgn_upd & cs)
        else {
                dc_select_clip_rgn(dc, rgn_upd);
                wm_cursor_paint(cs, dc);
        }
        wm.screen_state = WIN_INIT;
}

static void wm_create_bg_win(void)
{
        struct wm_window *bg_win;
        bg_win = create_wm_window();
        wm_window_resize(bg_win, 0, 0, wm.w - 1, wm.h - 1);
        PDIB wallpaper = load_dib(WALLPAPER_FILE);
        if (wallpaper == NULL) {
                GUI_WARN("load wallpaper file %s error\n", WALLPAPER_FILE);
                set_dc_fill_color(bg_win->dc, rgba_from_u32(COLOR_SkyBlue));
                fill_rect(bg_win->dc, 0, 0, bg_win->new_rc.r, bg_win->new_rc.b);
        } else {
                PBITMAP wallpaper_bitmap_origon =
                        create_dib_bitmap(wm.dc, wallpaper);
                PBITMAP wallpaper_bitmap =
                        create_compat_bitmap(wm.dc, wm.w, wm.h);
                int origin_w = get_bitmap_width(wallpaper_bitmap_origon);
                int origin_h = get_bitmap_height(wallpaper_bitmap_origon);
                dc_select_bitmap(bg_win->dc, wallpaper_bitmap);
                stretch_bitmap(bg_win->dc,
                               0,
                               0,
                               wm.w,
                               wm.h,
                               wallpaper_bitmap_origon,
                               0,
                               0,
                               origin_w,
                               origin_h,
                               ROP_SRCCOPY);
                wm_window_set_bitmap(bg_win, wallpaper_bitmap);
        }
        bg_win->state = WIN_MOVED;

        list_append(&bg_win->znode, &wm.zlist);
        list_append(&bg_win->pnode, &wm.plist);
}

inline void init_winmgr(void)
{
        wm_window_init_rgn();
        rgn_clip = create_region(32);
        rgn_upd = create_region(32);
        rgn_win = create_region(32);
        rgn_win2 = create_region(32);
        wm.cs = create_wm_cursor(0, 0, DEFAULT_CURSOR_WIDTH, DEFAULT_CURSOR_HEIGHT);
        init_list_head(&wm.zlist);
        init_list_head(&wm.plist);

        wm.dc = create_dc(DC_DEVICE);
        wm.screen_state = WIN_UNINIT;
}

inline void wm_set_screen_bitmap(PBITMAP screen_bitmap)
{
        wm.screen_bitmap = screen_bitmap;
        dc_select_bitmap(wm.dc, screen_bitmap);
        wm.w = get_bitmap_width(screen_bitmap);
        wm.h = get_bitmap_height(screen_bitmap);
        wm.depth = get_bitmap_depth(screen_bitmap);
        wm.pitch = get_bitmap_pitch(screen_bitmap);
        GUI_DEBUG("[GUI] HDMI: [%d %d %d %d]\n", wm.w, wm.h, wm.depth, wm.pitch);

        wm.screen_state = WIN_MOVED;
        wm_create_bg_win();
        set_rect(&wm.full_screen_rect, 0, 0, wm.w - 1, wm.h - 1);
        wm_cursor_create_bitmap(wm.cs, wm.dc);
}

/* Cursor window should not in the normal window list. It is managed independently.*/
void wm_set_cursor_win(struct wm_window *win)
{
        list_del(&win->znode);
        list_del(&win->pnode);
}

void wm_commit_cursor_win(struct wm_window *win)
{
        wm.cs_win = win;
        wm.cs->dc_cursor = win->dc;
        wm.cs->h = get_dc_height(win->dc);
        wm.cs->w = get_dc_width(win->dc);
        wm.cs->changed = true;
}

void wm_set_cursor_to_default()
{
        GUI_DEBUG("wm set cursor to default\n");
        set_cursor_to_default(wm.cs);
        wm.cs->changed = true;
}

inline void wm_add_win(struct wm_window *win)
{
        list_add(&win->znode, &wm.zlist);
        list_add(&win->pnode, &wm.plist);
        win->parent = NULL;
}

inline void wm_rm_win(struct wm_window *win)
{
        struct wm_window *son;
        if (!list_empty(&win->son_list)) {
                for_each_in_list (
                        son, struct wm_window, pnode, &win->son_list) {
                        wm_rm_win(son);
                }
        }
        list_del(&win->pnode);
        list_del(&win->znode);
}

void wm_win_change_parent(struct wm_window *parent, struct wm_window *son)
{
        list_del(&son->pnode);
        if (parent == NULL) {
                list_add(&son->pnode, &wm.plist);
        } else {
                list_add(&son->pnode, &parent->son_list);
        }
        son->parent = parent;
}

struct wm_window *wm_get_win_by_pos(int x, int y)
{
	struct wm_window *win;

        if (x < 0 || x > wm.w - 1 || y < 0 || y > wm.h - 1)
                return NULL;
        for_each_in_list (win, struct wm_window, znode, &wm.zlist) {
                if (pt_in_rect(&win->new_rc, x, y))
                        return win;
        }
        return NULL;
}

void wm_clear_top_win(void) {
        wm.top_win = NULL;
}

static void set_top_win_iter(struct wm_window *win)
{
        struct wm_window *son;
        
        if (!list_empty(&win->son_list)) {
                for_each_in_list_reverse (
                        son, struct wm_window, pnode, &win->son_list) {
                        set_top_win_iter(son);
                }
        }
        list_del(&win->znode);
        list_add(&win->znode, &wm.zlist);
        win->state = WIN_MOVED;
        GUI_DEBUG("Set top win: ");
        GUI_DEBUG_PRINT_RECT(&win->rc);
}

void wm_set_top_win(struct wm_window *win)
{

        if (win->parent != NULL) {
                wm_set_top_win(win->parent);
                return;
        }

        if (wm.top_win == win)
                return;
                
        set_top_win_iter(win);
        win->state = WIN_MOVED;
        list_del(&win->pnode);
        list_add(&win->pnode, &wm.plist);
        wm.top_win = win;

        /* Following are previous implementation, more complicated, but not tested. */

        // struct wm_window *front_win;
        // if (win->state == WIN_INIT || win->state == WIN_PAINTED) {
        //         win->state = WIN_PAINTED;
        //         // rgn_upd = 0
        //         clear_region(rgn_upd);
        //         // rgn_win = win->new_rc
        //         set_rc_region(rgn_win, &win->new_rc);
        //         for (front_win = container_of(
        //                      win->znode.prev, struct wm_window, znode);
        //              &front_win->znode != &wm.zlist;
        //              front_win = container_of(
        //                      front_win->znode.prev, struct wm_window, znode)) {
        //                 // rgn_win2 = front_win->new_rc & win->new_rc
        //                 set_rc_region(rgn_win2, &front_win->new_rc);
        //                 combine_region(rgn_win2, rgn_win2, rgn_win, RGN_AND);
        //                 // rgn_upd |= rgn_win2
        //                 if (!region_null(rgn_win2)) {
        //                         combine_region(
        //                                 rgn_upd, rgn_upd, rgn_win2, RGN_OR);
        //                 }
        //         }

        //         // win->rgn_upd |= rgn_upd'
        //         if (!region_null(rgn_upd)) {
        //                 win->state = WIN_PAINTED;
        //                 move_region(rgn_upd, -win->new_rc.l, -win->new_rc.t);
        //                 combine_region(win->rgn_upd_tmp,
        //                                win->rgn_upd,
        //                                rgn_upd,
        //                                RGN_OR);
        //         }
        // }
        
}


inline struct wm_cursor *wm_get_cursor(void)
{
	return wm.cs;
}

/*
 * 0 <= (x + *dx) < w
 * 0 <= (y + *dy) < h
 */
inline void wm_border_fix(int x, int y, int *dx, int *dy)
{
	if (x + *dx < 0)
		*dx = -x;
	else if (x + *dx >= wm.w)
		*dx = wm.w - 1 - x;
	if (y + *dy < 0)
		*dy = -y;
	else if (y + *dy >= wm.h)
		*dy = wm.h - 1 - y;
}

uint64_t wm_get_screen_size()
{
        return (((uint64_t)wm.h) << 32) | (uint64_t)wm.w;
}

/* Do window composition */
inline void wm_comp(void)
{
	wm_process_updates();
	wm_repaint();
}
