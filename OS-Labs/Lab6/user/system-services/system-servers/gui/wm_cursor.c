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

#include "wm_cursor.h"
#include <wayland-cursor.h>
#include "default_cursor.h"
#include <malloc.h>

#define BMP_CURSOR_FILE NULL

static PDC default_cursor;

/* Return 0 if succ */
static int create_wm_cursor_dc_from_disk(struct wm_cursor *cs, const PDC dc,
                                         char *filename)
{
        PDIB dib_cs;
        PBITMAP bitmap_cs;
        PBITMAP bitmap_cs2;
        int w, h;

        /* Load struct dibitmap */
        dib_cs = load_dib(filename);
        if (dib_cs == NULL) {
                return -1;
        }

        bitmap_cs = create_dib_bitmap(dc, dib_cs);
        bitmap_cs2 = create_compat_bitmap(dc, cs->w, cs->h);
        /* Select bitmap */
        dc_select_bitmap(cs->dc_cursor, bitmap_cs2);

        /* Stretch bitmap */
        w = get_bitmap_width(bitmap_cs);
        h = get_bitmap_height(bitmap_cs);
        stretch_bitmap(cs->dc_cursor,
                       0,
                       0,
                       cs->w,
                       cs->h,
                       bitmap_cs,
                       0,
                       0,
                       w,
                       h,
                       ROP_SRCCOPY);

        /* Delete struct dibitmap and bitmap */
        delete_dib(dib_cs);
        delete_bitmap(bitmap_cs);

        default_cursor = cs->dc_cursor;

        return 0;
}

/* Return 0 if succ */
static int create_wm_cursor_dc_from_local(struct wm_cursor *cs, const PDC dc)
{
        PBITMAP bitmap_cs;

        bitmap_cs = create_bitmap_from_vaddr(DEFAULT_CURSOR_WIDTH,
                                             DEFAULT_CURSOR_HEIGHT,
                                             32,
                                             DEFAULT_CURSOR_WIDTH * 4,
                                             default_cursor_data);

        /* Select bitmap */
        dc_select_bitmap(cs->dc_cursor, bitmap_cs);
        default_cursor = cs->dc_cursor;
        cs->w = DEFAULT_CURSOR_WIDTH;
        cs->h = DEFAULT_CURSOR_HEIGHT;

        return 0;
}

void set_cursor_to_default(struct wm_cursor *cs)
{
        cs->w = DEFAULT_CURSOR_HEIGHT;
        cs->h = DEFAULT_CURSOR_HEIGHT;
        cs->dc_cursor = default_cursor;
}

/*
 * Create a wm_cursor compatible with dc.
 * Create the bitmap and the mask bitmap for the wm_cursor.
 */
struct wm_cursor *create_wm_cursor(int x, int y, int w, int h)
{
        struct wm_cursor *cs = (struct wm_cursor *)malloc(sizeof(*cs));
        cs->x = x;
        cs->y = y;
        cs->w = w;
        cs->h = h;
        cs->new_x = x;
        cs->new_y = y;

        cs->dc_cursor = create_dc(DC_MEM);
        return cs;
}

void del_wm_cursor(struct wm_cursor *cs)
{
        if (cs->dc_cursor != NULL)
                delete_dc(cs->dc_cursor);
        free(cs);
}

inline void wm_cursor_create_bitmap(struct wm_cursor *cs, const PDC dc)
{
        if (create_wm_cursor_dc_from_disk(cs, dc, BMP_CURSOR_FILE) != 0)
                create_wm_cursor_dc_from_local(cs, dc);
        set_cursor_to_default(cs);
}

void wm_cursor_create_bitmap_from_buffer(struct wm_cursor *cs, const PDC dc,
                                         PBITMAP bitmap, void *buffer)
{
        /* Select bitmap */
        dc_select_bitmap(cs->dc_cursor, bitmap);
        cs->w = get_bitmap_width(bitmap);
        cs->h = get_bitmap_height(bitmap);
        cs->changed = true;
}

inline void wm_cursor_paint(struct wm_cursor *cs, PDC dc)
{
        blend_rgba(dc,
                   cs->x,
                   cs->y,
                   cs->w,
                   cs->h,
                   cs->dc_cursor,
                   0,
                   0,
                   cs->w,
                   cs->h,
                   255);
}

inline void wm_cursor_move(struct wm_cursor *cs, int dx, int dy)
{
        cs->new_x += dx;
        cs->new_y += dy;
}
