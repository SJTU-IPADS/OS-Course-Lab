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

#include <stdlib.h>
#include <string.h>
#include <stdnoreturn.h>
#include <pthread.h>
#include <unistd.h>
#include <chcore/type.h>
#include <chcore/syscall.h>
#include <chcore/pthread.h>
#include <chcore/memory.h>
#include <chcore/container/hashtable.h>
#include <wayland-client.h>

#include "terminal.h"
#include "schrift.h"
#include "libtsm.h"
#include "xkbcommon-keysyms.h"
#include "util.h"

#define JETBRAIN_MONO_TTF_SIZE 142628
extern const u8 jetbrain_mono_ttf[JETBRAIN_MONO_TTF_SIZE];

extern const u32 keymap_us[128][2];

#define DEFAULT_TTF            jetbrain_mono_ttf
#define DEFAULT_TTF_SIZE       JETBRAIN_MONO_TTF_SIZE
#define DEFAULT_FONT_SIZE      16
#define DEFAULT_MAX_SCROLLBACK 500

#define DEFAULT_KEYMAP keymap_us

#define GLYPH_HTABLE_SIZE          23
#define KEY_BUFFER_SIZE            6
#define REPAINTING_DURATION        10000
#define KEYBOARD_HANDLING_DURATION 10000
#define LONGPRESS_THRESHOLD        50
#define LONGPRESS_FREQUENCY        5

#define KEYPAD_FIRST 0x53
#define KEYPAD_LAST  0x63

#define SHIFT_MASK    (1 << 0)
#define CAP_LOCK_MASK (1 << 1)
#define CTRL_MASK     (1 << 2)
#define ALT_MASK      (1 << 3)
#define NUM_LOCK_MASK (1 << 4)

#define BUTTON_LEFT   0x110
#define BUTTON_RIGHT  0x111
#define BUTTON_MIDDLE 0x112

struct terminal {
        /* Wayland */
        struct wl_display *display;
        struct wl_registry *registry;
        struct wl_compositor *compositor;
        struct wl_shm *shm;
        struct wl_shell *shell;
        struct wl_seat *seat;
        struct wl_pointer *pointer;
        struct wl_keyboard *keyboard;
        struct wl_shm_pool *pool;
        struct wl_buffer *buffer;
        struct wl_surface *surface;
        struct wl_shell_surface *shell_surface;

        /* Font */
        SFT sft;
        struct htable glyphs;

        /* Terminal state machine */
        struct tsm_screen *screen;
        struct tsm_vte *vte;
        tsm_age_t age;

        /* Buffer */
        void *data;
        u32 width;
        u32 height;
        u32 stride;
        u32 cell_width;
        u32 cell_height;

        /* Keyboard */
        u32 mods_depressed;
        u32 mods_latched;
        u32 mods_locked;
        u32 keys[KEY_BUFFER_SIZE];
        u32 key_timers[KEY_BUFFER_SIZE];
        u32 key_num;

        /* Others */
        bool repaint;
        pthread_mutex_t mutex;
        send_to_shell_func_t send_to_shell;
};

struct glyph {
        u32 codepoint;
        SFT_Image image;
        SFT_GMetrics gmtx;
        struct hlist_node hnode;
};

static void vte_write(struct tsm_vte *vte, const char buffer[], size_t size,
                      void *data)
{
        struct terminal *terminal = data;

        /* There may be output to STDOUT in send_to_shell, which requires the
         * terminal mutex, so vte_write does not hold the mutex during
         * send_to_shell to avoid deadlock. */
        pthread_mutex_unlock(&terminal->mutex);

        terminal->send_to_shell(buffer, size);

        pthread_mutex_lock(&terminal->mutex);
}

static void pointer_enter_handler(void *data, struct wl_pointer *pointer,
                                  u32 serial, struct wl_surface *surface,
                                  wl_fixed_t surface_x, wl_fixed_t surface_y)
{
}

static void pointer_leave_handler(void *data, struct wl_pointer *pointer,
                                  u32 serial, struct wl_surface *surface)
{
}

static void pointer_motion_handler(void *data, struct wl_pointer *pointer,
                                   u32 time, wl_fixed_t surface_x,
                                   wl_fixed_t surface_y)
{
}

static void pointer_button_handler(void *data, struct wl_pointer *wl_pointer,
                                   u32 serial, u32 time, u32 button, u32 state)
{
        struct terminal *terminal = data;

        if (button == BUTTON_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED) {
                wl_shell_surface_set_toplevel(terminal->shell_surface);
                wl_shell_surface_move(
                        terminal->shell_surface, terminal->seat, serial);
        }
}

static void pointer_axis_handler(void *data, struct wl_pointer *wl_pointer,
                                 u32 time, u32 axis, wl_fixed_t value)
{
        struct terminal *terminal = data;
        int num = wl_fixed_to_int(value);

        if (num == 0)
                return;

        pthread_mutex_lock(&terminal->mutex);

        if (num < 0) {
                tsm_screen_sb_down(terminal->screen, -num);
        } else {
                tsm_screen_sb_up(terminal->screen, num);
        }

        terminal->repaint = true;

        pthread_mutex_unlock(&terminal->mutex);
}

static const struct wl_pointer_listener pointer_listener = {
        pointer_enter_handler,
        pointer_leave_handler,
        pointer_motion_handler,
        pointer_button_handler,
        pointer_axis_handler,
};

static void keyboard_keymap_hanlder(void *data, struct wl_keyboard *wl_keyboard,
                                    u32 format, s32 fd, u32 size)
{
}

static void keyboard_enter_handler(void *data, struct wl_keyboard *wl_keyboard,
                                   u32 serial, struct wl_surface *surface,
                                   struct wl_array *keys)
{
}

static void keyboard_leave_handler(void *data, struct wl_keyboard *wl_keyboard,
                                   u32 serial, struct wl_surface *surface)
{
        struct terminal *terminal = data;

        pthread_mutex_lock(&terminal->mutex);

        terminal->mods_depressed = 0;
        terminal->mods_latched = 0;
        terminal->mods_locked = 0;
        terminal->key_num = 0;

        pthread_mutex_unlock(&terminal->mutex);
}

static void keyboard_key_handler(void *data, struct wl_keyboard *wl_keyboard,
                                 u32 serial, u32 time_, u32 key, u32 state)
{
        struct terminal *terminal = data;
        u32 i;

        pthread_mutex_lock(&terminal->mutex);

        if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
                if (terminal->key_num == KEY_BUFFER_SIZE) {
                        pthread_mutex_unlock(&terminal->mutex);
                        return;
                }

                for (i = 0; i < terminal->key_num; i++) {
                        if (terminal->keys[i] == key) {
                                pthread_mutex_unlock(&terminal->mutex);
                                return;
                        }
                }

                terminal->keys[terminal->key_num] = key;
                terminal->key_timers[terminal->key_num] = 0;
                terminal->key_num++;
        } else {
                for (i = 0; i < terminal->key_num; i++) {
                        if (terminal->keys[i] == key) {
                                memmove(&terminal->keys[i],
                                        &terminal->keys[i + 1],
                                        (KEY_BUFFER_SIZE - i - 1)
                                                * sizeof terminal->keys[0]);
                                memmove(&terminal->key_timers[i],
                                        &terminal->key_timers[i + 1],
                                        (KEY_BUFFER_SIZE - i
                                         - 1) * sizeof terminal->key_timers[0]);
                                terminal->key_num--;
                                break;
                        }
                }
        }

        pthread_mutex_unlock(&terminal->mutex);
}

static void keyboard_modifiers_handler(void *data,
                                       struct wl_keyboard *wl_keyboard,
                                       u32 serial, u32 mods_depressed,
                                       u32 mods_latched, u32 mods_locked,
                                       u32 group)
{
        struct terminal *terminal = data;

        pthread_mutex_lock(&terminal->mutex);

        terminal->mods_depressed = mods_depressed;
        terminal->mods_latched = mods_latched;
        terminal->mods_locked = mods_locked;

        pthread_mutex_unlock(&terminal->mutex);
}

static void keyboard_repeat_info_handler(void *data,
                                         struct wl_keyboard *wl_keyboard,
                                         s32 rate, s32 delay)
{
}

static const struct wl_keyboard_listener keyboard_listener = {
        keyboard_keymap_hanlder,
        keyboard_enter_handler,
        keyboard_leave_handler,
        keyboard_key_handler,
        keyboard_modifiers_handler,
        keyboard_repeat_info_handler,
};

static void seat_capabilities_handler(void *data, struct wl_seat *seat,
                                      u32 capabilities)
{
        u32 cap_pointer, cap_keyboard;
        struct terminal *terminal = data;

        cap_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
        cap_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

        if (cap_pointer && terminal->pointer == NULL) {
                terminal->pointer = wl_seat_get_pointer(seat);
                wl_pointer_add_listener(
                        terminal->pointer, &pointer_listener, terminal);
        } else if (!cap_pointer && terminal->pointer != NULL) {
                wl_pointer_destroy(terminal->pointer);
                terminal->pointer = NULL;
        }

        if (cap_keyboard && terminal->keyboard == NULL) {
                terminal->keyboard = wl_seat_get_keyboard(seat);
                wl_keyboard_add_listener(
                        terminal->keyboard, &keyboard_listener, terminal);
        } else if (!cap_keyboard && terminal->keyboard != NULL) {
                wl_keyboard_destroy(terminal->keyboard);
                terminal->keyboard = NULL;
        }
}

static const struct wl_seat_listener seat_listener = {
        seat_capabilities_handler,
};

static void registry_global_handler(void *data, struct wl_registry *registry,
                                    u32 name, const char *interface,
                                    u32 version)
{
        struct terminal *terminal = data;

        if (strcmp(interface, "wl_compositor") == 0) {
                terminal->compositor = wl_registry_bind(
                        registry, name, &wl_compositor_interface, 4);
        } else if (strcmp(interface, "wl_shm") == 0) {
                terminal->shm =
                        wl_registry_bind(registry, name, &wl_shm_interface, 1);
        } else if (strcmp(interface, "wl_shell") == 0) {
                terminal->shell = wl_registry_bind(
                        registry, name, &wl_shell_interface, 1);
        } else if (strcmp(interface, "wl_seat") == 0
                   && terminal->seat == NULL) {
                terminal->seat =
                        wl_registry_bind(registry, name, &wl_seat_interface, 7);
                wl_seat_add_listener(terminal->seat, &seat_listener, terminal);
        }
}

static void registry_global_remove_handler(void *data,
                                           struct wl_registry *registry,
                                           u32 name)
{
}

static const struct wl_registry_listener registry_listener = {
        registry_global_handler,
        registry_global_remove_handler,
};

struct terminal *terminal_create(u32 col_num, u32 row_num,
                                 send_to_shell_func_t send_to_shell)
{
        struct terminal *terminal;
        SFT_Glyph gid;
        SFT_LMetrics lmtx;
        SFT_GMetrics gmtx;
        u32 buffer_size;
        cap_t pmo;

        check_ptr(terminal = malloc(sizeof *terminal));

        terminal->compositor = NULL;
        terminal->shm = NULL;
        terminal->shell = NULL;
        terminal->seat = NULL;
        terminal->pointer = NULL;
        terminal->keyboard = NULL;
        terminal->mods_depressed = 0;
        terminal->mods_latched = 0;
        terminal->mods_locked = 0;
        terminal->key_num = 0;
        terminal->repaint = true;
        terminal->send_to_shell = send_to_shell;
        pthread_mutex_init(&terminal->mutex, NULL);

        check_ptr(terminal->sft.font =
                          sft_loadmem(DEFAULT_TTF, DEFAULT_TTF_SIZE));
        terminal->sft.xScale = DEFAULT_FONT_SIZE;
        terminal->sft.yScale = DEFAULT_FONT_SIZE;

        init_htable(&terminal->glyphs, GLYPH_HTABLE_SIZE);
        check_ptr(terminal->glyphs.buckets);

        check_ret(tsm_screen_new(&terminal->screen, NULL, NULL));
        check_ret(tsm_screen_resize(terminal->screen, col_num, row_num));
        tsm_screen_set_max_sb(terminal->screen, DEFAULT_MAX_SCROLLBACK);

        check_ret(tsm_vte_new(&terminal->vte,
                              terminal->screen,
                              vte_write,
                              terminal,
                              NULL,
                              NULL));

        terminal->age = 0;

        tsm_screen_ref(terminal->screen);
        tsm_vte_ref(terminal->vte);

        sft_lmetrics(&terminal->sft, &lmtx);
        sft_lookup(&terminal->sft, 'A', &gid);
        sft_gmetrics(&terminal->sft, gid, &gmtx);
        terminal->cell_width = gmtx.advanceWidth;
        terminal->cell_height =
                2 * (lmtx.ascender + lmtx.descender + lmtx.lineGap);

        terminal->width = col_num * terminal->cell_width;
        terminal->height = row_num * terminal->cell_height;
        terminal->stride = terminal->width * 4;
        buffer_size = terminal->height * terminal->stride;

        terminal->display = wl_display_connect(NULL);
        while (terminal->display == NULL) {
                usys_yield();
                terminal->display = wl_display_connect(NULL);
        }

        check_ptr(terminal->registry =
                          wl_display_get_registry(terminal->display));
        wl_registry_add_listener(
                terminal->registry, &registry_listener, terminal);

        while (terminal->compositor == NULL || terminal->shm == NULL
               || terminal->shell == NULL) {
                wl_display_dispatch(terminal->display);
        }

        check_ptr(terminal->surface =
                          wl_compositor_create_surface(terminal->compositor));
        check_ptr(terminal->shell_surface = wl_shell_get_shell_surface(
                          terminal->shell, terminal->surface));

        check_ret(pmo = usys_create_pmo(buffer_size, PMO_DATA));
        check_ptr(terminal->data = chcore_auto_map_pmo(
                          pmo, buffer_size, VM_READ | VM_WRITE));
        check_ptr(terminal->pool =
                          wl_shm_create_pool(terminal->shm, pmo, buffer_size));
        check_ptr(terminal->buffer =
                          wl_shm_pool_create_buffer(terminal->pool,
                                                    0,
                                                    terminal->width,
                                                    terminal->height,
                                                    terminal->stride,
                                                    WL_SHM_FORMAT_XRGB8888));

        return terminal;
}

static void terminal_paint_color_cell(struct terminal *terminal, u32 cwidth,
                                      u32 posx, u32 posy, u8 r, u8 g, u8 b)
{
        u32 *dst;
        u32 i, j;
        u32 x, y, width, height;
        u32 xrgb;

        x = posx * terminal->cell_width;
        y = posy * terminal->cell_height;
        width = terminal->cell_width * cwidth;
        height = terminal->cell_height;

        xrgb = (0xff << 24) | (r << 16) | (g << 8) | b;

        for (j = x; j < x + width; j++) {
                for (i = y; i < y + height; i++) {
                        dst = terminal->data + i * terminal->stride + j * 4;
                        *dst = xrgb;
                }
        }
}

static struct glyph *terminal_lookup_glyph(struct terminal *terminal,
                                           u32 codepoint)
{
        struct glyph *glyph;
        struct hlist_head *bucket;
        SFT_Glyph gid;
        SFT_Image image;
        SFT_GMetrics gmtx;

        bucket = htable_get_bucket(&terminal->glyphs, codepoint);
        for_each_in_hlist (glyph, hnode, bucket) {
                if (glyph->codepoint == codepoint) {
                        return glyph;
                }
        }

        check_ret(sft_lookup(&terminal->sft, codepoint, &gid));
        check_ret(sft_gmetrics(&terminal->sft, gid, &gmtx));

        image.width = (gmtx.minWidth + 3) & ~3;
        image.height = gmtx.minHeight;
        check_ptr(image.pixels = malloc(image.width * image.height));

        check_ret(sft_render(&terminal->sft, gid, image));

        check_ptr(glyph = malloc(sizeof *glyph));
        glyph->codepoint = codepoint;
        glyph->image = image;
        glyph->gmtx = gmtx;
        init_hlist_node(&glyph->hnode);

        htable_add(&terminal->glyphs, codepoint, &glyph->hnode);

        return glyph;
}

static void terminal_paint_glyph_cell(struct terminal *terminal, u32 codepoint,
                                      u32 cwidth, u32 posx, u32 posy, u8 fr,
                                      u8 fg, u8 fb, u8 br, u8 bg, u8 bb)
{
        u8 *src;
        u32 *dst;
        u32 i, j;
        u32 x, y, width, height;
        u32 r, g, b, a, xrgb;
        u32 x_image, y_image;
        s32 i_image, j_image;
        struct glyph *glyph;
        SFT_LMetrics lmtx;

        x = posx * terminal->cell_width;
        y = posy * terminal->cell_height;
        width = cwidth * terminal->cell_width;
        height = terminal->cell_height;

        sft_lmetrics(&terminal->sft, &lmtx);
        glyph = terminal_lookup_glyph(terminal, codepoint);

        x_image = x + glyph->gmtx.leftSideBearing;
        y_image =
                y + lmtx.ascender - glyph->gmtx.yOffset - glyph->gmtx.minHeight;

        for (j = x; j < x + width; j++) {
                for (i = y; i < y + height; i++) {
                        dst = terminal->data + i * terminal->stride + j * 4;

                        i_image = glyph->image.height + y_image - i - 1;
                        j_image = j - x_image;

                        if (i_image >= 0 && i_image < glyph->image.height
                            && j_image >= 0 && j_image < glyph->image.width) {
                                src = glyph->image.pixels
                                      + i_image * glyph->image.width + j_image;
                                a = *src;
                        } else {
                                a = 0;
                        }

                        if (a == 0) {
                                r = br;
                                g = bg;
                                b = bb;
                        } else if (a == 255) {
                                r = fr;
                                g = fg;
                                b = fb;
                        } else {
                                r = fr * a + br * (255 - a);
                                r += 0x80;
                                r = (r + (r >> 8)) >> 8;

                                g = fg * a + bg * (255 - a);
                                g += 0x80;
                                g = (g + (g >> 8)) >> 8;

                                b = fb * a + bb * (255 - a);
                                b += 0x80;
                                b = (b + (b >> 8)) >> 8;
                        }

                        xrgb = (0xff << 24) | (r << 16) | (g << 8) | b;

                        *dst = xrgb;
                }
        }
}

static int paint_cell(struct tsm_screen *screen, u32 id, const u32 *codepoints,
                      size_t len, u32 cwidth, u32 posx, u32 posy,
                      const struct tsm_screen_attr *attr, tsm_age_t age,
                      void *data)
{
        u8 fr, fg, fb, br, bg, bb;
        struct terminal *terminal = data;

        if (age != 0 && terminal->age != 0 && age <= terminal->age) {
                return 0;
        }

        if (attr->inverse) {
                fr = attr->br;
                fg = attr->bg;
                fb = attr->bb;
                br = attr->fr;
                bg = attr->fg;
                bb = attr->fb;
        } else {
                fr = attr->fr;
                fg = attr->fg;
                fb = attr->fb;
                br = attr->br;
                bg = attr->bg;
                bb = attr->bb;
        }

        if (len == 0) {
                terminal_paint_color_cell(
                        terminal, cwidth, posx, posy, br, bg, bb);
        } else if (len == 1) {
                terminal_paint_glyph_cell(terminal,
                                          codepoints[0],
                                          cwidth,
                                          posx,
                                          posy,
                                          fr,
                                          fg,
                                          fb,
                                          br,
                                          bg,
                                          bb);
        } else {
                return -1;
        }

        return 0;
}

static void terminal_repaint(struct terminal *terminal)
{
        pthread_mutex_lock(&terminal->mutex);

        if (!terminal->repaint) {
                pthread_mutex_unlock(&terminal->mutex);
                return;
        }

        terminal->age = tsm_screen_draw(terminal->screen, paint_cell, terminal);

        wl_surface_attach(terminal->surface, terminal->buffer, 0, 0);
        wl_surface_damage_buffer(terminal->surface, 0, 0, INT32_MAX, INT32_MAX);
        wl_surface_commit(terminal->surface);
        wl_display_flush(terminal->display);

        terminal->repaint = false;

        pthread_mutex_unlock(&terminal->mutex);
}

static noreturn void *repainting_loop(void *arg)
{
        struct terminal *terminal = arg;

        for (;;) {
                terminal_repaint(terminal);
                usleep(REPAINTING_DURATION);
        }
}

static void terminal_handle_keyboard(struct terminal *terminal)
{
        u32 i, key, mods, keysym, ascii, unicode, key_timer;

        pthread_mutex_lock(&terminal->mutex);

        for (i = 0; i < terminal->key_num; i++) {
                key_timer = ++terminal->key_timers[i];

                if (key_timer < LONGPRESS_THRESHOLD) {
                        if (key_timer != 1) {
                                continue;
                        }
                } else {
                        key_timer -= LONGPRESS_THRESHOLD;
                        if (key_timer % LONGPRESS_FREQUENCY != 0) {
                                continue;
                        }
                }

                key = terminal->keys[i];

                mods = ((terminal->mods_depressed | terminal->mods_latched)
                        & (CTRL_MASK | ALT_MASK | SHIFT_MASK))
                       | (terminal->mods_locked
                          & (CAP_LOCK_MASK | NUM_LOCK_MASK));

                if (key >= KEYPAD_FIRST && key <= KEYPAD_LAST) {
                        keysym = DEFAULT_KEYMAP[key & 0x7f]
                                               [!!(mods & NUM_LOCK_MASK)];
                } else {
                        keysym = DEFAULT_KEYMAP[key & 0x7f]
                                               [!!(mods & SHIFT_MASK)
                                                ^ !!(mods & CAP_LOCK_MASK)];
                }

                if (keysym >= 0 && keysym < 0x80) {
                        ascii = unicode = keysym;
                } else {
                        ascii = XKB_KEY_NoSymbol;
                        unicode = TSM_VTE_INVALID;
                }

                if (keysym == XKB_KEY_Page_Up || keysym == XKB_KEY_KP_Page_Up) {
                        tsm_screen_sb_up(terminal->screen, 1);
                        terminal->repaint = true;
                } else if (keysym == XKB_KEY_Page_Down
                           || keysym == XKB_KEY_KP_Page_Down) {
                        tsm_screen_sb_down(terminal->screen, 1);
                        terminal->repaint = true;
                } else {
                        tsm_vte_handle_keyboard(
                                terminal->vte, keysym, ascii, mods, unicode);
                }
        }

        pthread_mutex_unlock(&terminal->mutex);
}

static noreturn void *keyboard_handling_loop(void *arg)
{
        struct terminal *terminal = arg;

        for (;;) {
                terminal_handle_keyboard(terminal);
                usleep(KEYBOARD_HANDLING_DURATION);
        }
}

noreturn void terminal_run(struct terminal *terminal)
{
        pthread_t tid;

        wl_shell_surface_set_toplevel(terminal->shell_surface);

        /* Create a thread to repaint the terminal periodically. The terminal
         * does not repaint every time its state is updated to avoid repainting
         * too frequently. */
        check_ret(chcore_pthread_create(&tid, NULL, repainting_loop, terminal));

        /* Create a thread to handle keyboard input periodically. The Wayland
         * compositor only sends key events when keys are pressed or released.
         * To implement long-press, the thread keeps polling the array of
         * currently pressed keys. */
        check_ret(chcore_pthread_create(
                &tid, NULL, keyboard_handling_loop, terminal));

        /* Wayland main loop */
        for (;;) {
                wl_display_dispatch(terminal->display);
        }
}

void terminal_put(struct terminal *terminal, const char buffer[], size_t size)
{
        pthread_mutex_lock(&terminal->mutex);

        tsm_screen_sb_reset(terminal->screen);
        tsm_vte_input(terminal->vte, buffer, size);
        terminal->repaint = true;

        pthread_mutex_unlock(&terminal->mutex);
}
