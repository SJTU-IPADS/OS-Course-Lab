#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <chcore/syscall.h>
#include <chcore/memory.h>
#include <wayland-client.h>

extern const int UI_WIDTH;
extern const int UI_HEIGHT;

void startPokerSlide(void *phy_fb, int width, int height, int color_bytes);
void sendTouch2PokerSlide(int x, int y, bool down);

static wl_compositor *compositor = NULL;
static wl_shm *shm = NULL;
static wl_shell *shell = NULL;
static wl_seat *seat = NULL;
static wl_pointer *pointer = NULL;
static wl_buffer *buffer = NULL;
static wl_surface *surface = NULL;
static wl_shell_surface *shell_surface = NULL;

static bool touch_down = false;
static int touch_x = 0;
static int touch_y = 0;

static void pointer_enter_handler(void *data, wl_pointer *pointer,
                                  uint32_t serial, wl_surface *surface,
                                  wl_fixed_t surface_x, wl_fixed_t surface_y)
{
}

static void pointer_leave_hanlder(void *data, wl_pointer *pointer,
                                  uint32_t serial, wl_surface *surface)
{
}

static void pointer_motion_handler(void *data, wl_pointer *pointer,
                                   uint32_t time, wl_fixed_t surface_x,
                                   wl_fixed_t surface_y)
{
        touch_x = wl_fixed_to_int(surface_x);
        touch_y = wl_fixed_to_int(surface_y);

        if (touch_down) {
                sendTouch2PokerSlide(touch_x, touch_y, touch_down);
                wl_surface_attach(surface, buffer, 0, 0);
                wl_surface_damage_buffer(surface, 0, 0, UI_WIDTH, UI_HEIGHT);
                wl_surface_commit(surface);
        }
}

static void pointer_button_handler(void *data, wl_pointer *wl_pointer,
                                   uint32_t serial, uint32_t time,
                                   uint32_t button, uint32_t state)
{
        if (button == 0x110 && state == 1) {
                wl_shell_surface_set_toplevel(shell_surface);
                wl_shell_surface_move(shell_surface, seat, serial);
        }

        if (button == 0x111 && state == 0 && touch_down) {
                touch_down = false;
                sendTouch2PokerSlide(touch_x, touch_y, touch_down);
                wl_surface_attach(surface, buffer, 0, 0);
                wl_surface_damage_buffer(surface, 0, 0, UI_WIDTH, UI_HEIGHT);
                wl_surface_commit(surface);
        }

        if (button == 0x111 && state == 1) {
                touch_down = true;
                sendTouch2PokerSlide(touch_x, touch_y, touch_down);
                wl_surface_attach(surface, buffer, 0, 0);
                wl_surface_damage_buffer(surface, 0, 0, UI_WIDTH, UI_HEIGHT);
                wl_surface_commit(surface);
        }
}

static void pointer_axis_handler(void *data, wl_pointer *wl_pointer,
                                 uint32_t time, uint32_t axis, wl_fixed_t value)
{
}

static const wl_pointer_listener pointer_listener = {
        pointer_enter_handler,
        pointer_leave_hanlder,
        pointer_motion_handler,
        pointer_button_handler,
        pointer_axis_handler,
};

static void seat_capabilities_handler(void *data, wl_seat *seat,
                                      uint32_t capabilities)
{
        uint32_t cap_pointer;

        cap_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;

        if (cap_pointer && pointer == NULL) {
                pointer = wl_seat_get_pointer(seat);
                wl_pointer_add_listener(pointer, &pointer_listener, NULL);
        } else if (!cap_pointer && pointer != NULL) {
                wl_pointer_destroy(pointer);
                pointer = NULL;
        }
}

static const wl_seat_listener seat_listener = {
        seat_capabilities_handler,
};

static void registry_global_handler(void *data, wl_registry *registry,
                                    uint32_t name, const char *interface,
                                    uint32_t version)
{
        if (strcmp(interface, "wl_compositor") == 0) {
                compositor = (wl_compositor *)wl_registry_bind(
                        registry, name, &wl_compositor_interface, 3);
        } else if (strcmp(interface, "wl_shm") == 0) {
                shm = (wl_shm *)wl_registry_bind(
                        registry, name, &wl_shm_interface, 1);
        } else if (strcmp(interface, "wl_shell") == 0) {
                shell = (wl_shell *)wl_registry_bind(
                        registry, name, &wl_shell_interface, 1);
        } else if (strcmp(interface, "wl_seat") == 0) {
                seat = (wl_seat *)wl_registry_bind(
                        registry, name, &wl_seat_interface, 1);
                wl_seat_add_listener(seat, &seat_listener, NULL);
        }
}

static void registry_global_remove_handler(void *data, wl_registry *registry,
                                           uint32_t name)
{
}

static const wl_registry_listener registry_listener = {
        registry_global_handler,
        registry_global_remove_handler,
};

int main()
{
        wl_display *display = wl_display_connect(NULL);
        wl_registry *registry = wl_display_get_registry(display);

        wl_registry_add_listener(registry, &registry_listener, NULL);

        while (compositor == NULL || shm == NULL || shell == NULL
               || pointer == NULL) {
                wl_display_dispatch(display);
        }

        surface = wl_compositor_create_surface(compositor);
        shell_surface = wl_shell_get_shell_surface(shell, surface);

        wl_shell_surface_set_toplevel(shell_surface);

        int width = UI_WIDTH;
        int height = UI_HEIGHT;
        int stride = width * 4;
        int size = stride * height;

        int pmo = usys_create_pmo(size, PMO_DATA);
        void *data = chcore_auto_map_pmo(pmo, size, VM_READ | VM_WRITE);

        wl_shm_pool *pool = wl_shm_create_pool(shm, pmo, size);
        buffer = wl_shm_pool_create_buffer(
                pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);

        startPokerSlide(data, width, height, 4);

        wl_surface_attach(surface, buffer, 0, 0);
        wl_surface_damage_buffer(surface, 0, 0, UI_WIDTH, UI_HEIGHT);
        wl_surface_commit(surface);

        while (true) {
                wl_display_dispatch(display);
        }
}
