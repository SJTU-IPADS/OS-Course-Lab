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

#include "wayland_backend.h"

#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <chcore/ipc.h>
#include <chcore/container/list.h>

#include <hashmap.h>
#include <libpipe.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include "winmgr.h"
#include "wm_cursor.h"
#include "wm_window.h"
#include "default_cursor.h"

#include "guilog/guilog.h"

// As two parameters of a function
#define DEFAULT_WINDOW_POSITION_X 200
#define DEFAULT_WINDOW_POSITION_Y 200

struct pointer;

static uint32_t cursor_serial = 0;
static uint32_t operate_serial = 0;

struct surface {
        struct wl_resource *res;
        struct wl_client *c;
        struct wm_window *win;
        struct wl_resource *buffer;
        uint8_t lock;
        enum { SURFACE_WIN,
               SURFACE_CURSOR,
        } type;
        /* For delete surface safely. */
        bool deleted;
        int refcount;
        /* Following are data for cursor surface. */
        struct {
                int hotspot_x;
                int hotspot_y;
                struct pointer *p; // for double buffer
                uint32_t serial;
        } cursor_data;
};

struct keyboard {
        struct wl_list res_list; // resources in wl_clients
        struct surface *focus;
        struct {
                u32 mods_depressed;
                u32 mods_latched;
                u32 mods_locked;
                u32 keys[6];
        } state;
};

struct pointer {
        struct wl_list res_list; // resources in wl_clients
        struct wm_cursor *cs;
        struct surface *focus; // pointer locates on, NULL on background
        struct surface *move;
        struct {
                u32 buttons;
        } state;
        int hotspot_x;
        int hotspot_y;
        struct wm_window *win;
};

struct seat {
        struct pointer *pointer;
        struct keyboard *keyboard;
};

struct wayland_backend {
        /* client_badge -> wl_client
         * Exists previously, but not used at present. 
         */
        struct hashmap *badge_wc_map;
        struct wl_display *display;
        struct seat *seat;
        struct wl_event_loop *loop;
        struct wl_global *wl_compositor;
        struct wl_global *wl_subcompositor;
        struct wl_global *wl_shell;
        struct wl_global *wl_seat;
        struct wl_global *xdg_wm_base;
        pthread_mutex_t wlb_lock;
};

static struct wayland_backend wlb = {0};

static inline struct surface *get_surface_by_pos(int x, int y)
{
        struct wm_window *win = wm_get_win_by_pos(x, y);
        if (win == NULL)
                return NULL;
        return win->surf;
}

static int get_pointer_hotspotx(struct pointer *p)
{
        return p->cs->new_x + p->hotspot_x;
}

static int get_pointer_hotspoty(struct pointer *p)
{
        return p->cs->new_y + p->hotspot_y;
}

static struct keyboard *create_keyboard(void)
{
        struct keyboard *k = malloc(sizeof(*k));
        wl_list_init(&k->res_list);
        k->focus = NULL;
        k->state.mods_depressed = 0;
        k->state.mods_latched = 0;
        k->state.mods_locked = 0;
        memset(k->state.keys, 0, sizeof(k->state.keys));
        return k;
}

static struct pointer *create_pointer(void)
{
        struct pointer *p = malloc(sizeof(*p));
        wl_list_init(&p->res_list);
        p->cs = wm_get_cursor();
        p->focus = NULL;
        p->move = NULL;
        p->state.buttons = 0;
        p->hotspot_x = 6;
        p->hotspot_y = 6;
        return p;
}

static struct seat *create_seat(void)
{
        struct seat *s = malloc(sizeof(*s));
        s->keyboard = create_keyboard();
        s->pointer = create_pointer();
        return s;
}

/* Happens when the pointer click in a new surface. */
static void keyboard_change_focus(struct surface *surf)
{
        struct keyboard *k = wlb.seat->keyboard;
        struct wl_resource *r_keyboard;
        struct wl_array keys;
        u32 *key;
        u32 i;
        if (k->focus == surf) {
                return;
        }
        if (k->focus && k->focus->deleted) {
                k->focus->refcount--;
                if (k->focus->refcount == 0) {
                        free(k->focus);
                }
                k->focus = NULL;
        }
        if (k->focus != NULL) {
                r_keyboard =
                        wl_resource_find_for_client(&k->res_list, k->focus->c);
                if (r_keyboard == NULL) {
                        GUI_WARN("r_keyboard == NULL\n");
                        goto next;
                }
                wl_keyboard_send_leave(
                        r_keyboard, operate_serial++, k->focus->res);
        }

next:
        if (surf != NULL) {
                if (surf->win->type.type == WIN_TYPE_TRANSIENT
                    && surf->win->type.type_flags.transient_flags.transient_flag
                               == WL_SHELL_SURFACE_TRANSIENT_INACTIVE) {
                        return;
                }
                r_keyboard = wl_resource_find_for_client(&k->res_list, surf->c);
                if (r_keyboard == NULL) {
                        GUI_WARN("r_keyboard == NULL\n");
                        surf = NULL;
                        goto next_out;
                }

                wl_array_init(&keys);
                for (i = 0; i < 6; i++) {
                        if (k->state.keys[i] != 0) {
                                key = wl_array_add(&keys, sizeof(*key));
                                *key = k->state.keys[i];
                        }
                }
                GUI_DEBUG("keyboard focus %p\n", surf->res);
                wl_keyboard_send_enter(
                        r_keyboard, operate_serial++, surf->res, &keys);
                wl_keyboard_send_modifiers(r_keyboard,
                                           0,
                                           k->state.mods_depressed,
                                           k->state.mods_latched,
                                           k->state.mods_locked,
                                           0);
                wl_array_release(&keys);
        }

next_out:
        k->focus = surf;
        if (surf)
                surf->refcount++;
}

static void pointer_change_focus(struct surface *surf)
{
        int px, py;
        struct pointer *p = wlb.seat->pointer;
        struct wl_resource *r_pointer_leave = NULL;
        struct wl_resource *r_pointer_enter = NULL;
        px = get_pointer_hotspotx(p);
        py = get_pointer_hotspoty(p);
        surf = get_surface_by_pos(px, py);
        if (p->focus == surf) {
                return;
        }
        /* If the pointer is not on the surface, it should be on the background.
         */
        if (p->focus != NULL) {
                r_pointer_leave =
                        wl_resource_find_for_client(&p->res_list, p->focus->c);
                if (r_pointer_leave == NULL) {
                        GUI_WARN("r_pointer_leave == NULL\n");
                        wl_pointer_send_leave(r_pointer_leave,
                                              operate_serial++,
                                              p->focus->res);
                        GUI_DEBUG("send leave to %p\n", p->focus->res);
                }
        }
        if (surf != NULL) {
                if (p->focus) {
                        GUI_DEBUG("from: ");
                        GUI_DEBUG_PRINT_RECT(&p->focus->win->rc);
                }
                GUI_DEBUG("to: ");
                GUI_DEBUG_PRINT_RECT(&surf->win->rc);
                r_pointer_enter =
                        wl_resource_find_for_client(&p->res_list, surf->c);
                if (r_pointer_enter == NULL) {
                        GUI_DEBUG("r_pointer == NULL\n");
                        surf = NULL;
                        goto out;
                }
                wl_pointer_send_enter(
                        r_pointer_enter,
                        operate_serial++,
                        surf->res,
                        wl_fixed_from_int(px - surf->win->new_rc.l),
                        wl_fixed_from_int(py - surf->win->new_rc.t));
                GUI_DEBUG("send enter to %p, with x %d y %d\n",
                          surf->res,
                          wl_fixed_from_int(px - surf->win->new_rc.l),
                          wl_fixed_from_int(py - surf->win->new_rc.t));
        } else {
                // enter the background
                if (p->focus) {
                        GUI_DEBUG("from: ");
                        GUI_DEBUG_PRINT_RECT(&p->focus->win->rc);
                }
                GUI_DEBUG("to background\n");
                if (r_pointer_leave) {
                        wl_pointer_send_leave(r_pointer_leave,
                                              operate_serial++,
                                              p->focus->res);
                        GUI_DEBUG("send leave to %p\n", p->focus->res);
                }
                cursor_serial = operate_serial++;
                wm_set_cursor_to_default();
                wm_cursor_move(p->cs,
                               p->hotspot_x - DEFAULT_HOTSPOT_X,
                               p->hotspot_y - DEFAULT_HOTSPOT_Y);
                p->hotspot_x = DEFAULT_HOTSPOT_X;
                p->hotspot_y = DEFAULT_HOTSPOT_Y;
        }
out:
        if (p->focus) {
                p->focus->refcount--;
        }
        if (surf) {
                surf->refcount++;
        }
        p->focus = surf;
}

static void pointer_handle_raw_event(struct pointer *p, raw_pointer_event_t *e)
{
        u32 i, btn_released, btn_pressed;
        u32 button_masks[] = {1 << 0, 1 << 1, 1 << 2};
        struct wl_resource *r_pointer = NULL;
        int dx, dy, px, py, px_0, py_0;
        struct surface *surf = NULL;

        btn_released = p->state.buttons & ~e->buttons;
        btn_pressed = ~p->state.buttons & e->buttons;

        p->state.buttons = e->buttons;

        if (p->focus && p->focus->deleted) {
                p->focus->refcount--;
                if (p->focus->refcount == 0) {
                        free(p->focus);
                }
                p->focus = NULL;
                cursor_serial = operate_serial++;
                wm_set_cursor_to_default();
                wm_cursor_move(p->cs,
                               p->hotspot_x - DEFAULT_HOTSPOT_X,
                               p->hotspot_y - DEFAULT_HOTSPOT_Y);
                p->hotspot_x = DEFAULT_HOTSPOT_X;
                p->hotspot_y = DEFAULT_HOTSPOT_Y;
        }
        if (p->focus != NULL) {
                r_pointer =
                        wl_resource_find_for_client(&p->res_list, p->focus->c);
        }

        if (e->buttons == 0 && p->move != NULL) {
#ifdef XDG_SHELL_ENABLED
                xdg_surface_send_configure(p->move->res, 0);
#endif
                p->move = NULL;
        }
        dx = e->x;
        dy = e->y;

//Question TODO
        if (dx != 0 || dy != 0 || p->focus == NULL) {
                /* Get old hotspot */
                px_0 = get_pointer_hotspotx(p);
                py_0 = get_pointer_hotspoty(p);
                wm_border_fix(px_0, py_0, &dx, &dy);

                // 1、移动屏幕上光标的位置，你可以使用wm_cursor_move()函数。
                

                /* Move window */
                if (p->move != NULL) {
                        // 2、移动窗口，你可以使用wm_root_window_move()函数。其中p->move->win指向要移动的窗口。

                }

                px = get_pointer_hotspotx(p);
                py = get_pointer_hotspoty(p);
                surf = get_surface_by_pos(px, py);
                /* Focus NOT changed */
                if (p->focus == surf && surf != NULL && r_pointer != NULL) {
                        // 3、光标在窗口内移动，向窗口发送正确的事件。你需要使用wl_fixed_from_int(px - surf->win->new_rc.l)得到光标焦点在窗口内的x坐标，y坐标同理。
                        

                } else {
                        // 4、光标移动跨越窗口，调用pointer_change_focus()函数以更新光标的focus窗口。这其中包含了光标在背景内移动的情况，安心，此时pointer_change_focus()不会做任何事情。
                        

                }
        }

        if (p->focus != NULL) {
                r_pointer =
                        wl_resource_find_for_client(&p->res_list, p->focus->c);
                if (r_pointer == NULL) {
                        GUI_DEBUG("r_pointer == NULL\n");
                        return;
                }

                for (i = 0; i < 3; i++) {
                        if (btn_pressed & button_masks[i]) {
                                // 5、设置键盘聚焦窗口的变化，以及将当前窗口置于顶层。你可以使用keyboard_change_focus()函数以及wm_set_top_win()函数。

                                
                                break;
                        }
                }

                /* Handle button released */
                for (i = 0; i < 3; i++) {
                        if (btn_released & button_masks[i]) {
                                // 6、发送鼠标释放事件。其中键码应当使用0x110 + i。



                        }
                }
                /* Handle button pressed */
                for (i = 0; i < 3; i++) {
                        if (btn_pressed & button_masks[i]) {
                                // 7、发送鼠标按下事件。其中键码应当使用0x110 + i。


                                
                        }
                }
                /* Handle scroll */
                if (e->scroll != 0) {
                        wl_pointer_send_axis(r_pointer,
                                             0,
                                             WL_POINTER_AXIS_VERTICAL_SCROLL,
                                             wl_fixed_from_int(e->scroll));
                }
        }
}

static void keys_substract(u32 lhs[6], u32 rhs[6], u32 out[6])
{
        u32 i, j;
        u32 k = 0;
        bool found;

        memset(out, 0, 6 * sizeof(out[0]));

        for (i = 0; i < 6; i++) {
                if (lhs[i] == 0)
                        continue;

                found = false;
                for (j = 0; j < 6; j++) {
                        if (lhs[i] == rhs[j]) {
                                found = true;
                                break;
                        }
                }
                if (!found)
                        out[k++] = lhs[i];
        }
}

static void keyboard_handle_raw_event(struct keyboard *k,
                                      raw_keyboard_event_t *e)
{
        bool mods_changed;
        u32 keys_pressed[6];
        u32 keys_released[6];
        u32 i;
        struct wl_resource *r_keyboard;

// Question TODO
        // 阅读下面的四行代码（465-468），理解我们是怎样维护键盘状态的，依照维护的键盘状态和新的键盘事件，得到按下和释放的按键，以及修饰键是否改变。
        // 你可以使用 static void keys_substract(u32 lhs[6], u32 rhs[6], u32 out[6]) 函数，其输入为两个按键数组，输出为按键数组的差集。（{key | key in lhs and key not in rhs}）




        k->state.mods_depressed = e->mods_depressed;
        k->state.mods_latched = e->mods_latched;
        k->state.mods_locked = e->mods_locked;
        memcpy(k->state.keys, e->keys, sizeof(e->keys));
        if (k->focus && k->focus->deleted) {
                k->focus->refcount--;
                if (k->focus->refcount == 0) {
                        free(k->focus);
                }
                k->focus = NULL;
        }
        if (k->focus != NULL) {
                r_keyboard =
                        wl_resource_find_for_client(&k->res_list, k->focus->c);
                if (r_keyboard == NULL)
                        return;
// Question TODO
                // 阅读Wayland接口文档，在下面三个空白处填入对应的函数调用，以实现键盘事件的发送。其中serial参数你可以使用operate_serial++。
                for (i = 0; i < 6 && keys_pressed[i] != 0; i++) {
                // 空白1


                }

                for (i = 0; i < 6 && keys_released[i] != 0; i++) {
                // 空白2


                }

                if (mods_changed) {
                // 空白3


                }
        }
}

static void wl_surface_destroy(struct wl_client *c, struct wl_resource *r)
{
        struct surface *surf = wl_resource_get_user_data(r);
        GUI_DEBUG("destroy surface %p\n", r);
        surf->win->state = WIN_REMOVED;
        surf->deleted = true;
        wl_resource_destroy(r);
}

static void wl_surface_attach(struct wl_client *c, struct wl_resource *r,
                              struct wl_resource *r_buffer, int x, int y)
{
        struct surface *surf;
        struct wl_shm_buffer *shm_buffer;
        PBITMAP bitmap;
        int width, height, depth, pitch;

        surf = wl_resource_get_user_data(r);
        shm_buffer = wl_shm_buffer_get(r_buffer);
        width = wl_shm_buffer_get_width(shm_buffer);
        height = wl_shm_buffer_get_height(shm_buffer);
        pitch = wl_shm_buffer_get_stride(shm_buffer);
        depth = pitch / width * 8;

        bitmap = create_bitmap_from_vaddr(width,
                                          height,
                                          depth,
                                          pitch,
                                          wl_shm_buffer_get_data(shm_buffer));
        surf->win->buffer_attached = r_buffer;
        wm_window_set_attached_bitmap(surf->win, bitmap);
}

/* When surface_attachc or surface_damage is called, just record them. And when
 * surface_commit is called, copy them to the screen. */
static void wl_surface_damage(struct wl_client *c, struct wl_resource *r, int x,
                              int y, int width, int height)
{
        struct surface *surf = wl_resource_get_user_data(r);
        wm_window_damage(surf->win, x, y, x + width - 1, y + height - 1);
}

static void wl_surface_frame(struct wl_client *c, struct wl_resource *r,
                             u32 callback)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static void wl_surface_set_opaque_region(struct wl_client *c,
                                         struct wl_resource *r,
                                         struct wl_resource *r_region)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static void wl_surface_set_input_region(struct wl_client *c,
                                        struct wl_resource *r,
                                        struct wl_resource *r_region)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static void wl_surface_commit(struct wl_client *c, struct wl_resource *r)
{
        struct surface *surf = wl_resource_get_user_data(r);
        if (surf->type == SURFACE_WIN) {
                wm_window_paint(surf->win);
                if (surf->win->buffer_attached) {
                        wl_buffer_send_release(surf->win->buffer_attached);
                        surf->win->buffer_attached = NULL;
                }
                surf->win->type = surf->win->type_tmp;
        } else if (surf->type == SURFACE_CURSOR) {
                GUI_DEBUG(
                        "oprate serial is %d, now cursor is on %dst operate and"
                        " the commited one is on %dst operate.\n",
                        operate_serial,
                        cursor_serial,
                        surf->cursor_data.serial);
                wm_window_paint(surf->win);
                if (surf->cursor_data.serial >= cursor_serial) {
                        /* If the cursor is not the newest change, ignore to
                         * display it. */
                        wm_commit_cursor_win(surf->win);
                        
                        cursor_serial = surf->cursor_data.serial;
                        wm_cursor_move(surf->cursor_data.p->cs,
                                       surf->cursor_data.p->hotspot_x
                                               - surf->cursor_data.hotspot_x,
                                       surf->cursor_data.p->hotspot_y
                                               - surf->cursor_data.hotspot_y);
                        surf->cursor_data.p->hotspot_x =
                                surf->cursor_data.hotspot_x;
                        surf->cursor_data.p->hotspot_y =
                                surf->cursor_data.hotspot_y;
                }
                if (surf->win->buffer_attached) {
                        wl_buffer_send_release(surf->win->buffer_attached);
                        surf->win->buffer_attached = NULL;
                }
        }
}

static void wl_surface_set_buffer_transform(struct wl_client *c,
                                            struct wl_resource *r,
                                            int transform)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static void wl_surface_set_buffer_scale(struct wl_client *c,
                                        struct wl_resource *r, int scale)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static void wl_surface_damage_buffer(struct wl_client *c, struct wl_resource *r,
                                     int x, int y, int width, int height)
{
        struct surface *surf = wl_resource_get_user_data(r);
        wm_window_damage(surf->win, x, y, x + width - 1, y + height - 1);
}

static struct wl_surface_interface wl_surface_implementation = {
        wl_surface_destroy,
        wl_surface_attach,
        wl_surface_damage,
        wl_surface_frame,
        wl_surface_set_opaque_region,
        wl_surface_set_input_region,
        wl_surface_commit,
        wl_surface_set_buffer_transform,
        wl_surface_set_buffer_scale,
        wl_surface_damage_buffer,
};

static void wl_subsurface_set_position(struct wl_client *client,
                                       struct wl_resource *resource, int32_t x,
                                       int32_t y)
{
        struct surface *surface = wl_resource_get_user_data(resource);
        if (surface->win->parent == NULL) {
                wl_client_post_implementation_error(client, "Not a subsurface");
        }
        int32_t new_x = surface->win->parent->new_rc.l + x;
        int32_t new_y = surface->win->parent->new_rc.t + y;
        wm_window_move(surface->win,
                       new_x - surface->win->new_rc.l,
                       new_y - surface->win->new_rc.t);
}

static void wl_subsurface_place_above(struct wl_client *client,
                                      struct wl_resource *resource,
                                      struct wl_resource *sibling)
{
        wl_client_post_implementation_error(client, "NOT implemented %s.", __func__);
}

static void wl_subsurface_place_below(struct wl_client *client,
                                      struct wl_resource *resource,
                                      struct wl_resource *sibling)
{
        wl_client_post_implementation_error(client, "NOT implemented %s.", __func__);
}

static void wl_subsurface_set_sync(struct wl_client *client,
                                   struct wl_resource *resource)
{
        wl_client_post_implementation_error(client, "NOT implemented %s.", __func__);
}

static void wl_subsurface_set_desync(struct wl_client *client,
                                     struct wl_resource *resource)
{
        /* We only support desync mode now, so we do nothing at set_desync, and
         * post an error at set_sync*/
}

static void wl_subsurface_destroy(struct wl_client *client,
                                  struct wl_resource *resource)
{
        struct surface *surface = wl_resource_get_user_data(resource);
        struct wm_window *win = surface->win;
        win->state = WIN_HIDDEN;
        wl_resource_destroy(resource);
}

static struct wl_subsurface_interface wl_subsurface_implmentation = {
        .set_position = wl_subsurface_set_position,
        .place_above = wl_subsurface_place_above,
        .place_below = wl_subsurface_place_below,
        .set_sync = wl_subsurface_set_sync,
        .set_desync = wl_subsurface_set_desync,
        .destroy = wl_subsurface_destroy,

};

static void wl_shell_surface_pong(struct wl_client *c, struct wl_resource *r,
                                  u32 serial)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static void wl_shell_surface_move(struct wl_client *c, struct wl_resource *r,
                                  struct wl_resource *r_seat, u32 serial)
{
        struct surface *surf;
        struct seat *s;

        surf = wl_resource_get_user_data(r);
        s = wl_resource_get_user_data(r_seat);

        if (s->pointer == NULL)
                return;

        s->pointer->move = surf;
}

static void wl_shell_surface_resize(struct wl_client *c, struct wl_resource *r,
                                    struct wl_resource *r_seat, u32 serial,
                                    u32 edges)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static void wl_shell_surface_set_toplevel(struct wl_client *c,
                                          struct wl_resource *r)
{
        struct surface *surf = wl_resource_get_user_data(r);
        if (surf->win->normal_rc) {
                wm_window_move(surf->win,
                               surf->win->normal_rc->l,
                               surf->win->normal_rc->t);
                if (surf->win->buffer_attached) {
                        wl_buffer_send_release(surf->win->buffer_attached);
                        surf->win->buffer_attached = NULL;
                }
                wl_shell_surface_send_configure(
                        r,
                        0,
                        surf->win->normal_rc->r - surf->win->normal_rc->l + 1,
                        surf->win->normal_rc->b - surf->win->normal_rc->t + 1);
        }
        surf->win->type_tmp.type = WIN_TYPE_TOPLEVEL;
        delete_rect(surf->win->normal_rc);
        surf->win->normal_rc = NULL;
}

static void wl_shell_surface_set_transient(struct wl_client *c,
                                           struct wl_resource *r,
                                           struct wl_resource *parent, int x,
                                           int y, u32 flags)
{
        struct surface *surf = wl_resource_get_user_data(r);
        surf->win->type_tmp.type = WIN_TYPE_TRANSIENT;
        surf->win->type_tmp.type_flags.transient_flags.parent =
                ((struct surface *)wl_resource_get_user_data(r))->win;
        surf->win->type_tmp.type_flags.transient_flags.transient_flag = flags;
        surf->win->type_tmp.type_flags.transient_flags.x = x;
        surf->win->type_tmp.type_flags.transient_flags.y = y;

        struct surface *par = wl_resource_get_user_data(parent);
        struct wm_window *son_win = surf->win;
        struct wm_window *par_win = par->win;
        wm_window_move(son_win,
                       par_win->rc.l - son_win->rc.l + x,
                       par_win->rc.b - son_win->rc.b + y);
        wm_win_change_parent(par_win, son_win);
}

/*NOTE: Setting fullscreen is dealt with by client at present, which works well
 * in LVGL. */
static void wl_shell_surface_set_fullscreen(struct wl_client *c,
                                            struct wl_resource *r, u32 method,
                                            u32 framerate,
                                            struct wl_resource *r_output)
{
        struct surface *surf = wl_resource_get_user_data(r);
        uint64_t screen_size = wm_get_screen_size();
        wm_window_move(surf->win, -surf->win->new_rc.l, -surf->win->new_rc.t);
        if (surf->win->buffer_attached) {
                wl_buffer_send_release(surf->win->buffer_attached);
                surf->win->buffer_attached = NULL;
        }
        wl_shell_surface_send_configure(r,
                                        0,
                                        GET_SCREEN_W(screen_size),
                                        GET_SCREEN_H(screen_size));
        surf->win->normal_rc = create_rect(surf->win->rc.l,
                                           surf->win->rc.t,
                                           surf->win->rc.r,
                                           surf->win->rc.b);
        surf->win->type_tmp.type = WIN_TYPE_FULLSCREEN;
}

static void wl_shell_surface_set_popup(struct wl_client *c,
                                       struct wl_resource *r,
                                       struct wl_resource *r_seat, u32 serial,
                                       struct wl_resource *parent, int x, int y,
                                       u32 flags)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static void wl_shell_surface_set_maximized(struct wl_client *c,
                                           struct wl_resource *r,
                                           struct wl_resource *r_output)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static void wl_shell_surface_set_title(struct wl_client *c,
                                       struct wl_resource *r, const char *title)
{
        /* We just do nothing at present, as it is required to display title bar by client. */
}

static void wl_shell_surface_set_class(struct wl_client *c,
                                       struct wl_resource *r,
                                       const char *class_)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static struct wl_shell_surface_interface wl_shell_surface_impl = {
        wl_shell_surface_pong,
        wl_shell_surface_move,
        wl_shell_surface_resize,
        wl_shell_surface_set_toplevel,
        wl_shell_surface_set_transient,
        wl_shell_surface_set_fullscreen,
        wl_shell_surface_set_popup,
        wl_shell_surface_set_maximized,
        wl_shell_surface_set_title,
        wl_shell_surface_set_class,
};

static void wl_pointer_set_cursor(struct wl_client *c, struct wl_resource *r,
                                  u32 serial, struct wl_resource *r_surface,
                                  int hotspot_x, int hotspot_y)
{
        struct pointer *p = wl_resource_get_user_data(r);
        if (p == NULL)
                return;
        struct surface *surf =
                ((struct surface *)wl_resource_get_user_data(r_surface));
        surf->type = SURFACE_CURSOR;
        surf->cursor_data.hotspot_x = hotspot_x;
        surf->cursor_data.hotspot_y = hotspot_y;
        surf->cursor_data.p = p;
        surf->cursor_data.serial = serial;
        wm_set_cursor_win(surf->win);
}

static void wl_pointer_release(struct wl_client *c, struct wl_resource *r)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static struct wl_pointer_interface wl_pointer_impl = {
        wl_pointer_set_cursor,
        wl_pointer_release,
};

static void wl_keyboard_release(struct wl_client *c, struct wl_resource *r)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static struct wl_keyboard_interface wl_keyboard_impl = {
        wl_keyboard_release,
};

static void wl_compositor_create_surface(struct wl_client *c,
                                         struct wl_resource *r, u32 id)
{
        struct wl_resource *r_surface;
        struct surface *surf;

        r_surface = wl_resource_create(c, &wl_surface_interface, 4, id);
        if (r_surface == NULL) {
                wl_client_post_no_memory(c);
                return;
        }

        surf = (struct surface *)malloc(sizeof(*surf));
        surf->res = r_surface;
        surf->c = c;
        surf->win = create_wm_window();
        surf->win->surf = surf;
        wm_window_move(surf->win,
                       DEFAULT_WINDOW_POSITION_X,
                       DEFAULT_WINDOW_POSITION_Y);
        surf->type = SURFACE_WIN;
        surf->lock = 0;
        surf->deleted = false;
        surf->refcount = 0;
        /* When a new window created, the next clicked window will be set as top
         * window, even if it is new created one, which is alreday on the top.
         */
        wm_clear_top_win();
        wl_resource_set_implementation(
                r_surface, &wl_surface_implementation, surf, NULL);
}

static void wl_compositor_create_region(struct wl_client *c,
                                        struct wl_resource *r, u32 id)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static struct wl_compositor_interface wl_compositor_impl = {
        wl_compositor_create_surface,
        wl_compositor_create_region,
};

static void wl_subcompositor_destroy(struct wl_client *c, struct wl_resource *r)
{
}

static void wl_subcompositor_get_subsurface(struct wl_client *c,
                                            struct wl_resource *r, u32 id,
                                            struct wl_resource *surface,
                                            struct wl_resource *parent)
{
        struct surface *son = wl_resource_get_user_data(surface);
        struct surface *par = wl_resource_get_user_data(parent);
        struct wm_window *son_win = son->win;
        struct wm_window *par_win = par->win;
        wm_add_win(son_win);
        wm_window_move(son_win,
                       par_win->rc.l - son_win->rc.l,
                       par_win->rc.b - son_win->rc.b);
        wm_win_change_parent(par_win, son_win);
        struct wl_resource *subsurface =
                wl_resource_create(c, &wl_subsurface_interface, 1, id);
        wl_resource_set_implementation(
                subsurface, &wl_subsurface_implmentation, son, NULL);
}

static struct wl_subcompositor_interface wl_subcompositor_implementation = {
        .destroy = wl_subcompositor_destroy,
        .get_subsurface = wl_subcompositor_get_subsurface,
};

static void wl_shell_get_shell_surface(struct wl_client *c,
                                       struct wl_resource *r, u32 id,
                                       struct wl_resource *r_surface)
{
        struct surface *surf;
        struct wl_resource *r_shell_surface;

        surf = wl_resource_get_user_data(r_surface);

        r_shell_surface =
                wl_resource_create(c, &wl_shell_surface_interface, 1, id);
        if (r_shell_surface == NULL) {
                wl_client_post_no_memory(c);
                return;
        }
        wl_resource_set_implementation(
                r_shell_surface, &wl_shell_surface_impl, surf, NULL);
        wm_add_win(surf->win);
}

static struct wl_shell_interface wl_shell_impl = {
        wl_shell_get_shell_surface,
};

static void wl_seat_get_pointer(struct wl_client *c, struct wl_resource *r,
                                u32 id)
{
        struct seat *s;
        struct pointer *p;
        struct wl_resource *r_pointer;

        s = wl_resource_get_user_data(r);
        p = s->pointer;

        if (p == NULL) {
                wl_resource_post_error(
                        r, WL_SEAT_ERROR_MISSING_CAPABILITY, "NO pointer");
                return;
        }

        r_pointer = wl_resource_create(c, &wl_pointer_interface, 7, id);
        if (r_pointer == NULL) {
                wl_client_post_no_memory(c);
                return;
        }

        wl_resource_set_implementation(r_pointer, &wl_pointer_impl, p, NULL);
        wl_list_insert(&p->res_list, wl_resource_get_link(r_pointer));
}

static void wl_seat_get_keyboard(struct wl_client *c, struct wl_resource *r,
                                 u32 id)
{
        struct seat *s;
        struct keyboard *k;
        struct wl_resource *r_keyboard;

        s = wl_resource_get_user_data(r);
        k = s->keyboard;

        if (k == NULL) {
                wl_resource_post_error(
                        r, WL_SEAT_ERROR_MISSING_CAPABILITY, "NO keyboard");
                return;
        }

        r_keyboard = wl_resource_create(c, &wl_keyboard_interface, 7, id);
        if (r_keyboard == NULL) {
                wl_client_post_no_memory(c);
                return;
        }

        wl_resource_set_implementation(r_keyboard, &wl_keyboard_impl, k, NULL);
        wl_list_insert(&k->res_list, wl_resource_get_link(r_keyboard));
}

static void wl_seat_get_touch(struct wl_client *c, struct wl_resource *r,
                              u32 id)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static void wl_seat_release(struct wl_client *c, struct wl_resource *r)
{
        wl_client_post_implementation_error(c, "NOT implemented %s.", __func__);
}

static struct wl_seat_interface wl_seat_impl = {
        wl_seat_get_pointer,
        wl_seat_get_keyboard,
        wl_seat_get_touch,
        wl_seat_release,
};

static void wl_subcompositor_bind(struct wl_client *c, void *data, u32 version,
                                  u32 id)
{
        struct wl_resource *r;

        r = wl_resource_create(c, &wl_subcompositor_interface, version, id);
        if (r == NULL) {
                wl_client_post_no_memory(c);
                return;
        }

        wl_resource_set_implementation(
                r, &wl_subcompositor_implementation, NULL, NULL);
}

static void wl_compositor_bind(struct wl_client *c, void *data, u32 version,
                               u32 id)
{
        struct wl_resource *r;

        r = wl_resource_create(c, &wl_compositor_interface, version, id);
        if (r == NULL) {
                wl_client_post_no_memory(c);
                return;
        }

        wl_resource_set_implementation(r, &wl_compositor_impl, NULL, NULL);
}

static void wl_shell_bind(struct wl_client *c, void *data, u32 version, u32 id)
{
        struct wl_resource *r;

        r = wl_resource_create(c, &wl_shell_interface, version, id);
        if (r == NULL) {
                wl_client_post_no_memory(c);
                return;
        }

        wl_resource_set_implementation(r, &wl_shell_impl, NULL, NULL);
}

static void wl_seat_bind(struct wl_client *c, void *data, u32 version, u32 id)
{
        struct wl_resource *r;
        struct seat *s = data;
        u32 capabilities = 0;
        GUI_DEBUG("seat bind\n");
        r = wl_resource_create(c, &wl_seat_interface, version, id);
        if (r == NULL) {
                wl_client_post_no_memory(c);
                return;
        }

        wl_resource_set_implementation(r, &wl_seat_impl, s, NULL);

        if (s->pointer != NULL)
                capabilities |= WL_SEAT_CAPABILITY_POINTER;
        if (s->keyboard != NULL)
                capabilities |= WL_SEAT_CAPABILITY_KEYBOARD;

        GUI_DEBUG("seat send caps\n");
        wl_seat_send_capabilities(r, capabilities);
}

static int keyboard_pipe_data(struct pipe *pp, u32 mask, void *data)
{
        struct keyboard *k = data;
        raw_keyboard_event_t e;

        while (pipe_read_exact(pp, &e, sizeof(e)) != 0) {
                keyboard_handle_raw_event(k, &e);
        }
        return 0;
}

static int pointer_pipe_data(struct pipe *pp, u32 mask, void *data)
{
        struct pointer *p = data;
        raw_pointer_event_t e;

        while (pipe_read_exact(pp, &e, sizeof(e)) != 0) {
                pointer_handle_raw_event(p, &e);
        }
        return 0;
}

void wayland_conn(ipc_msg_t *ipc_msg, badge_t client_badge)
{
        cap_t client_cap_group_cap, c2s_pipe_pmo, s2c_pipe_pmo;
        cap_t vsync_notific_cap;
        struct pipe *c2s_pipe, *s2c_pipe;

        client_cap_group_cap = ipc_get_msg_cap(ipc_msg, 0);
        c2s_pipe_pmo = ipc_get_msg_cap(ipc_msg, 1);
        s2c_pipe_pmo = ipc_get_msg_cap(ipc_msg, 2);
        vsync_notific_cap = ipc_get_msg_cap(ipc_msg, 3);
        if (client_cap_group_cap < 0 || c2s_pipe_pmo < 0 || s2c_pipe_pmo < 0
            || vsync_notific_cap < 0)
                ipc_return(ipc_msg, 0);

        c2s_pipe = create_pipe_from_pmo(PAGE_SIZE, c2s_pipe_pmo);
        s2c_pipe = create_pipe_from_pmo(PAGE_SIZE, s2c_pipe_pmo);
        pipe_init(c2s_pipe);
        pipe_init(s2c_pipe);

        pthread_mutex_lock(&wlb.wlb_lock);
        chcore_wl_client_create(wlb.display,
                                c2s_pipe,
                                s2c_pipe,
                                client_cap_group_cap,
                                vsync_notific_cap);
        pthread_mutex_unlock(&wlb.wlb_lock);

        ipc_set_msg_return_cap_num(ipc_msg, 1);
        ipc_set_msg_cap(ipc_msg, 0, SELF_CAP);
        ipc_return_with_cap(ipc_msg, 0);
}

inline void wayland_add_keyboard_pipe(struct pipe *keyboard_pipe)
{
        pthread_mutex_lock(&wlb.wlb_lock);
        chcore_wl_event_loop_add_pipe(wlb.loop,
                                      keyboard_pipe,
                                      WL_EVENT_READABLE,
                                      keyboard_pipe_data,
                                      wlb.seat->keyboard);
        pthread_mutex_unlock(&wlb.wlb_lock);
}

inline void wayland_add_mouse_pipe(struct pipe *mouse_pipe)
{
        pthread_mutex_lock(&wlb.wlb_lock);
        chcore_wl_event_loop_add_pipe(wlb.loop,
                                      mouse_pipe,
                                      WL_EVENT_READABLE,
                                      pointer_pipe_data,
                                      wlb.seat->pointer);
        pthread_mutex_unlock(&wlb.wlb_lock);
}

inline void wayland_handle_backend(void)
{
        pthread_mutex_lock(&wlb.wlb_lock);
        wl_display_flush_clients(wlb.display);
        wl_event_loop_dispatch(wlb.loop, -1);
        pthread_mutex_unlock(&wlb.wlb_lock);
}

/* Following codes are supporting xdg shells. They are not implemented at all. */
/* TODO: implement xdg shells. */
#define XDG_IMPLEMENTED 0
#if XDG_IMPLEMENTED
/*xdg_positioner, for popups*/
struct xdg_positioner {
        struct {
                int width;
                int height;
        } size;
        struct {
                int x;
                int y;
                int width;
                int height;
        } anchor_rect;
        unsigned int anchor;
        unsigned int gravity;
        struct wl_resource *r_positioner;
        struct wm_window *win;
};
static void xdg_positioner_destroy(struct wl_client *client,
                                   struct wl_resource *resource)
{
        wl_client_post_implementation_error(
                client, "xdg_surface is not implemented at present.");
}

static void xdg_positioner_set_size(struct wl_client *client,
                                    struct wl_resource *resource, int32_t width,
                                    int32_t height)
{
        wl_client_post_implementation_error(
                client, "xdg_surface is not implemented at present.");
}

static void xdg_positioner_set_anchor_rect(struct wl_client *client,
                                           struct wl_resource *resource,
                                           int32_t x, int32_t y, int32_t width,
                                           int32_t height)
{
        wl_client_post_implementation_error(
                client, "xdg_surface is not implemented at present.");
}

static void xdg_positioner_set_anchor(struct wl_client *client,
                                      struct wl_resource *resource,
                                      uint32_t anchor)
{
        wl_client_post_implementation_error(
                client, "xdg_surface is not implemented at present.");
}

static void xdg_positioner_set_gravity(struct wl_client *client,
                                       struct wl_resource *resource,
                                       uint32_t gravity)
{
        wl_client_post_implementation_error(
                client, "xdg_surface is not implemented at present.");
}

static void
xdg_positioner_set_constraint_adjustment(struct wl_client *client,
                                         struct wl_resource *resource,
                                         uint32_t constraint_adjustment)
{
        wl_client_post_implementation_error(
                client, "xdg_surface is not implemented at present.");
}

static void xdg_positioner_set_offset(struct wl_client *client,
                                      struct wl_resource *resource, int32_t x,
                                      int32_t y)
{
        wl_client_post_implementation_error(
                client, "xdg_surface is not implemented at present.");
}

static void xdg_positioner_set_reactive(struct wl_client *client,
                                        struct wl_resource *resource)
{
        wl_client_post_implementation_error(
                client, "xdg_surface is not implemented at present.");
}

static void xdg_positioner_set_parent_size(struct wl_client *client,
                                           struct wl_resource *resource,
                                           int32_t parent_width,
                                           int32_t parent_height)
{
        wl_client_post_implementation_error(
                client, "xdg_surface is not implemented at present.");
}

static void xdg_positioner_set_parent_configure(struct wl_client *client,
                                                struct wl_resource *resource,
                                                uint32_t serial)
{
        wl_client_post_implementation_error(
                client, "xdg_surface is not implemented at present.");
}

static struct xdg_positioner_interface xdg_positioner_impl = {
        xdg_positioner_destroy,
        xdg_positioner_set_size,
        xdg_positioner_set_anchor_rect,
        xdg_positioner_set_anchor,
        xdg_positioner_set_gravity,
        xdg_positioner_set_constraint_adjustment,
        xdg_positioner_set_offset,
        xdg_positioner_set_reactive,
        xdg_positioner_set_parent_size,
        xdg_positioner_set_parent_configure,
};

static void xdg_wm_base_create_positioner(struct wl_client *client,
                                          struct wl_resource *resource,
                                          uint32_t id)
{
        struct wl_resource *r_positioner;
        struct xdg_positioner *positioner =
                malloc(sizeof(struct xdg_positioner));
        r_positioner =
                wl_resource_create(client, &xdg_positioner_interface, 6, id);
        positioner->r_positioner = r_positioner;
        wl_resource_set_implementation(
                r_positioner, &xdg_positioner_impl, positioner, NULL);
}

/*xdg_surface*/

static void xdg_surface_destroy(struct wl_client *client,
                                struct wl_resource *resource)
{
        wl_client_post_implementation_error(
                client, "xdg_surface is not implemented at present.");
}

static void xdg_toplevel_move(struct wl_client *c, struct wl_resource *r,
                              struct wl_resource *r_seat, u32 serial)
{
        struct surface *surf;
        struct seat *s;

        surf = wl_resource_get_user_data(r);
        s = wl_resource_get_user_data(r_seat);

        if (s->pointer == NULL)
                return;

        if (pt_in_wm_win(surf->win,
                         get_pointer_hotspotx(s->pointer),
                         get_pointer_hotspoty(s->pointer)))
                s->pointer->move = surf;
}

static void xdg_toplevel_set_maximized(struct wl_client *client,
                                       struct wl_resource *resource)
{
        struct surface *surf = wl_resource_get_user_data(resource);
        struct wl_array *states = malloc(sizeof(struct wl_array));
        uint64_t screen_size = wm_get_screen_size();
        wm_window_move(surf->win, -surf->win->new_rc.l, -surf->win->new_rc.t);
        if (surf->win->buffer_attached) {
                wl_buffer_send_release(surf->win->buffer_attached);
                surf->win->buffer_attached = NULL;
        }
        
        wl_array_init(states);
        xdg_toplevel_send_configure(resource,
                                    GET_SCREEN_W(screen_size),
                                    GET_SCREEN_H(screen_size),
                                    states);
        surf->win->normal_rc = create_rect(surf->win->rc.l,
                                           surf->win->rc.t,
                                           surf->win->rc.r,
                                           surf->win->rc.b);
}

static void xdg_toplevel_unset_maximized(struct wl_client *client,
                                         struct wl_resource *resource)
{
        struct surface *surf = wl_resource_get_user_data(resource);
        struct wl_array *states = malloc(sizeof(struct wl_array));
        uint64_t screen_size = wm_get_screen_size();
        wm_window_move(surf->win, -surf->win->new_rc.l, -surf->win->new_rc.t);
        if (surf->win->buffer_attached) {
                wl_buffer_send_release(surf->win->buffer_attached);
                surf->win->buffer_attached = NULL;
        }
        
        wl_array_init(states);
        xdg_toplevel_send_configure(resource,
                                    GET_SCREEN_W(screen_size),
                                    GET_SCREEN_H(screen_size),
                                    states);
        surf->win->normal_rc = create_rect(surf->win->rc.l,
                                           surf->win->rc.t,
                                           surf->win->rc.r,
                                           surf->win->rc.b);
}

static struct xdg_toplevel_interface xdg_toplevel_impl = {
        .move = xdg_toplevel_move,
        .set_maximized = xdg_toplevel_set_maximized,
        .unset_maximized = xdg_toplevel_unset_maximized,
};
static void xdg_surface_get_toplevel(struct wl_client *client,
                                     struct wl_resource *resource, uint32_t id)
{
        struct surface *surf;
        struct wl_resource *r_toplevel;

        surf = wl_resource_get_user_data(resource);

        r_toplevel = wl_resource_create(client, &xdg_toplevel_interface, 6, id);
        if (r_toplevel == NULL) {
                wl_client_post_no_memory(client);
                return;
        }
        wl_resource_set_implementation(
                r_toplevel, &xdg_toplevel_impl, surf, NULL);
        wm_add_win(surf->win);
}

static void xdg_surface_get_popup(struct wl_client *client,
                                  struct wl_resource *resource, uint32_t id,
                                  struct wl_resource *parent,
                                  struct wl_resource *positioner)
{
        wl_client_post_implementation_error(
                client, "xdg_surface is not implemented at present.");
}
static struct xdg_surface_interface xdg_surface_impl = {
        xdg_surface_destroy,
        xdg_surface_get_toplevel,
        xdg_surface_get_popup};

static void xdg_wm_base_get_xdg_surface(struct wl_client *client,
                                        struct wl_resource *resourcer,
                                        uint32_t id,
                                        struct wl_resource *surface)
{
        struct surface *surf = wl_resource_get_user_data(surface);
        struct wl_resource *r_surf =
                wl_resource_create(client, &xdg_surface_interface, 6, id);
        wl_resource_set_implementation(r_surf, &xdg_surface_impl, surf, NULL);
}

static void xdg_wm_base_destroy(struct wl_client *client,
                                struct wl_resource *resource)
{
        wl_client_post_implementation_error(
                client, "xdg_surface is not implemented at present.");
}

void xdg_wm_base_pong(struct wl_client *client, struct wl_resource *resource,
                      uint32_t serial)
{
        wl_client_post_implementation_error(
                client, "xdg_surface is not implemented at present.");
}

static struct xdg_wm_base_interface xdg_wm_base_impl = {
        xdg_wm_base_destroy,
        xdg_wm_base_create_positioner,
        xdg_wm_base_get_xdg_surface,
        xdg_wm_base_pong};

static void xdg_wm_base_bind(struct wl_client *c, void *data, u32 version,
                             u32 id)
{
        struct wl_resource *r;

        r = wl_resource_create(c, &xdg_wm_base_interface, version, id);
        if (r == NULL) {
                wl_client_post_no_memory(c);
                return;
        }

        wl_resource_set_implementation(r, &xdg_wm_base_impl, NULL, NULL);
}

/* End of XDG Shell*/
#endif
void init_wayland_backend(void)
{
        wlb.badge_wc_map = create_hashmap(64);
        wlb.display = wl_display_create();
        wlb.seat = create_seat();
        wlb.loop = wl_display_get_event_loop(wlb.display);
        wlb.wl_compositor = wl_global_create(wlb.display,
                                             &wl_compositor_interface,
                                             4,
                                             NULL,
                                             wl_compositor_bind);
        wlb.wl_subcompositor = wl_global_create(wlb.display,
                                                &wl_subcompositor_interface,
                                                1,
                                                NULL,
                                                wl_subcompositor_bind);
        wlb.wl_shell = wl_global_create(
                wlb.display, &wl_shell_interface, 1, NULL, wl_shell_bind);
        wlb.wl_seat = wl_global_create(
                wlb.display, &wl_seat_interface, 7, wlb.seat, wl_seat_bind);
#if XDG_IMPLEMENTED
        wlb.xdg_wm_base = wl_global_create(
                wlb.display, &xdg_wm_base_interface, 6, NULL, xdg_wm_base_bind);
#endif
        wl_display_init_shm(wlb.display);
        pthread_mutex_init(&wlb.wlb_lock, NULL);
}