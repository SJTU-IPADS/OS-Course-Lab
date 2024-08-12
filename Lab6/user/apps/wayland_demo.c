
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

#include <stdio.h>
#include <string.h>

#include <wayland-client.h>

#include <chcore/syscall.h>
#include <chcore/memory.h>

struct wl_compositor *compositor = NULL;
struct wl_shm *shm = NULL;
struct wl_shell *shell = NULL;
struct wl_seat *seat = NULL;
struct wl_pointer *pointer = NULL;
struct wl_keyboard *keyboard = NULL;
struct wl_shell_surface *shell_surface = NULL;
struct wl_surface *surface = NULL;
struct wl_buffer *buffer = NULL;
unsigned char *buffer_data = NULL;
int square_state = 0;
bool argb_mode = false;

void transform(int state);

void keyboard_keymap_hanlder(void *data, struct wl_keyboard *wl_keyboard,
                             uint32_t format, int32_t fd, uint32_t size)
{
}

void keyboard_enter_handler(void *data, struct wl_keyboard *wl_keyboard,
                            uint32_t serial, struct wl_surface *surface,
                            struct wl_array *keys)
{
}

void keyboard_leave_handler(void *data, struct wl_keyboard *wl_keyboard,
                            uint32_t serial, struct wl_surface *surface)
{
}

void keyboard_key_handler(void *data, struct wl_keyboard *wl_keyboard,
                          uint32_t serial, uint32_t time, uint32_t key,
                          uint32_t state)
{
}

void keyboard_modifiers_handler(void *data, struct wl_keyboard *wl_keyboard,
                                uint32_t serial, uint32_t mods_depressed,
                                uint32_t mods_latched, uint32_t mods_locked,
                                uint32_t group)
{
}

void keyboard_repeat_info_handler(void *data, struct wl_keyboard *wl_keyboard,
                                  int32_t rate, int32_t delay)
{
}

const struct wl_keyboard_listener keyboard_listener = {
        keyboard_keymap_hanlder,
        keyboard_enter_handler,
        keyboard_leave_handler,
        keyboard_key_handler,
        keyboard_modifiers_handler,
        keyboard_repeat_info_handler,
};

void pointer_enter_handler(void *data, struct wl_pointer *pointer,
                           uint32_t serial, struct wl_surface *surface,
                           wl_fixed_t surface_x, wl_fixed_t surface_y)
{
}

void pointer_leave_hanlder(void *data, struct wl_pointer *pointer,
                           uint32_t serial, struct wl_surface *surface)
{
}

void pointer_motion_handler(void *data, struct wl_pointer *pointer,
                            uint32_t time, wl_fixed_t surface_x,
                            wl_fixed_t surface_y)
{
}

void pointer_button_handler(void *data, struct wl_pointer *wl_pointer,
                            uint32_t serial, uint32_t time, uint32_t button,
                            uint32_t state)
{
        if (button == 0x110 && state == 1 && shell_surface != NULL) {
                wl_shell_surface_set_toplevel(shell_surface);
                wl_shell_surface_move(shell_surface, seat, serial);
        } else if (button == 0x111 && state == 1 && shell_surface != NULL) {
                wl_shell_surface_set_toplevel(shell_surface);
                transform(square_state);
                square_state = !square_state;
        }
}

void pointer_axis_handler(void *data, struct wl_pointer *wl_pointer,
                          uint32_t time, uint32_t axis, wl_fixed_t value)
{
}

const struct wl_pointer_listener pointer_listener = {
        pointer_enter_handler,
        pointer_leave_hanlder,
        pointer_motion_handler,
        pointer_button_handler,
        pointer_axis_handler,
};

void seat_capabilities_handler(void *data, struct wl_seat *seat,
                               uint32_t capabilities)
{
        uint32_t cap_pointer, cap_keyboard;

        cap_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
        cap_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

        if (cap_pointer && pointer == NULL) {
                pointer = wl_seat_get_pointer(seat);
                wl_pointer_add_listener(pointer, &pointer_listener, NULL);
        } else if (!cap_pointer && pointer != NULL) {
                wl_pointer_destroy(pointer);
                pointer = NULL;
        }

        if (cap_keyboard && keyboard == NULL) {
                keyboard = wl_seat_get_keyboard(seat);
                wl_keyboard_add_listener(keyboard, &keyboard_listener, NULL);
        } else if (!cap_keyboard && keyboard != NULL) {
                wl_keyboard_destroy(keyboard);
                keyboard = NULL;
        }
}

const struct wl_seat_listener seat_listener = {
        seat_capabilities_handler,
};

void registry_global_handler(void *data, struct wl_registry *registry,
                             uint32_t name, const char *interface,
                             uint32_t version)
{
        if (strcmp(interface, "wl_compositor") == 0) {
                compositor = wl_registry_bind(
                        registry, name, &wl_compositor_interface, 3);
        } else if (strcmp(interface, "wl_shm") == 0) {
                shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
        } else if (strcmp(interface, "wl_shell") == 0) {
                shell = wl_registry_bind(
                        registry, name, &wl_shell_interface, 1);
        } else if (strcmp(interface, "wl_seat") == 0) {
                seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
                wl_seat_add_listener(seat, &seat_listener, NULL);
        }
}

void registry_global_remove_handler(void *data, struct wl_registry *registry,
                                    uint32_t name)
{
}

const struct wl_registry_listener registry_listener = {
        .global = registry_global_handler,
        .global_remove = registry_global_remove_handler,
};

int main(int argc, char *argv[])
{
        if (argc > 1 && !strcmp("-a", argv[1])) {
                printf("argb mode\n");
                argb_mode = true;
        } else {
                printf("xrgb mode\n");
                printf("You can run this app in argb mode with \"-a\" option\n");
        }

        struct wl_display *display = wl_display_connect(NULL);
        struct wl_registry *registry = wl_display_get_registry(display);

        wl_registry_add_listener(registry, &registry_listener, NULL);

        // wait for the "initial" set of globals to appear
        while (compositor == NULL || shm == NULL || shell == NULL
               || keyboard == NULL || pointer == NULL) {
                wl_display_dispatch(display);
        }

        surface = wl_compositor_create_surface(compositor);
        shell_surface = wl_shell_get_shell_surface(shell, surface);
        wl_shell_surface_set_toplevel(shell_surface);

        int width = 200;
        int height = 200;
        int stride = width * 4;
        int size = stride * height; // bytes

        // // open an anonymous file and write some zero bytes to it
        // int fd = syscall(SYS_memfd_create, "buffer", 0);
        // ftruncate(fd, size);

        // // map it to the memory
        // mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        int pmo = usys_create_pmo(size, PMO_DATA);
        buffer_data = chcore_auto_map_pmo(pmo, size, VM_READ | VM_WRITE);

        // turn it into a shared memory pool
        // struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
        struct wl_shm_pool *pool = wl_shm_create_pool(shm, pmo, size);

        // allocate the buffer in that pool
        // TODO: fix format
        if (argb_mode) {
                buffer = wl_shm_pool_create_buffer(
                        pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
        } else {
                buffer = wl_shm_pool_create_buffer(
                        pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
        }

        transform(square_state);
        square_state = !square_state;

        while (1) {
                wl_display_dispatch(display);
        }
}

void transform(int state)
{
        int width = 200;
        int height = 200;
        int stride = width * 4;

        if (!state) {
                for (int x = 0; x < width; x++) {
                        for (int y = 0; y < height; y++) {
                                struct pixel {
                                        unsigned blue : 8;
                                        unsigned green : 8;
                                        unsigned red : 8;
                                        unsigned alpha : 8; // dummy for xrgb
                                } *px = (struct pixel *)(buffer_data
                                                         + y * stride + x * 4);

                                if ((x + y) % 30 < 10) {
                                        // blue
                                        px->red = 0;
                                        px->green = 0;
                                        px->blue = 255;
                                        px->alpha = 255;
                                } else if ((x + y) % 30 < 20) {
                                        // yellow (semitransparent in argb)
                                        px->red = 255;
                                        px->green = 255;
                                        px->blue = 0;
                                        px->alpha = 128;
                                } else {
                                        // red (transparent in argb)
                                        px->red = 255;
                                        px->green = 0;
                                        px->blue = 0;
                                        px->alpha = 0;
                                }
                        }
                }
        } else {
                for (int x = 0; x < width; x++) {
                        for (int y = 0; y < height; y++) {
                                struct pixel {
                                        unsigned blue : 8;
                                        unsigned green : 8;
                                        unsigned red : 8;
                                        unsigned alpha : 8; // dummy for xrgb
                                } *px = (struct pixel *)(buffer_data
                                                         + y * stride + x * 4);

                                if ((x + y) % 30 < 10) {
                                        // green (semitransparent in argb)
                                        px->red = 0;
                                        px->green = 255;
                                        px->blue = 0;
                                        px->alpha = 128;
                                } else if ((x + y) % 30 < 20) {
                                        // purple
                                        px->red = 255;
                                        px->green = 0;
                                        px->blue = 255;
                                        px->alpha = 255;
                                } else {
                                        // cyan (transparent in argb)
                                        px->red = 0;
                                        px->green = 255;
                                        px->blue = 255;
                                        px->alpha = 0;
                                }
                        }
                }
        }

        wl_surface_attach(surface, buffer, 0, 0);
        wl_surface_damage_buffer(surface, 0, 0, width, height);
        wl_surface_commit(surface);
}