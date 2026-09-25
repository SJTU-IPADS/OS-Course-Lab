/**
 * @file lv_port_sdl2.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lvgl/lvgl.h"
#include "lv_drivers/wayland/wayland.h"


#include "port.h"
#include <unistd.h>
#include <time.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
static struct {
    void (*original_cb)(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
    uint32_t key;
    lv_indev_state_t state;
} __keyboard_state;

static void keyboard_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    __keyboard_state.original_cb(drv, data);
    if (data->key != __keyboard_state.key || data->state != __keyboard_state.state) {
        __keyboard_state.key = data->key;
        __keyboard_state.state = data->state;
        lv_port_keystate[data->key] = data->state;
    }
}

int lv_port_init(void)
{

    lv_disp_t* disp = lv_wayland_create_window(LV_SCREEN_HOR_RES, LV_SCREEN_VER_RES, "GBA Emulator", NULL);
    lv_disp_set_default(disp);
    lv_indev_t* mouse = lv_wayland_get_pointer(disp);
    // lv_indev_set_group(mouse, lv_group_get_default());
    // lv_indev_set_disp(mouse, disp);
    printf("3\n");
    // lv_indev_t* mousewheel = lv_sdl_mousewheel_create();
    // lv_indev_set_disp(mousewheel, disp);
    // lv_indev_set_group(mousewheel, lv_group_get_default());
    
    for(int i = 0; i < KEY_NUMS; i ++){
        lv_port_keystate[i] = 0;
    }
    lv_group_set_default(lv_group_create());
    lv_indev_t *keyboard = lv_wayland_get_keyboard(lv_disp_get_default());
    lv_indev_drv_t *keyboard_drv = keyboard->driver;
    __keyboard_state.original_cb = keyboard_drv->read_cb;
    keyboard_drv->read_cb = keyboard_read_cb;

    return 0;
}

void lv_port_sleep(uint32_t ms)
{
    usleep(ms * 1000);
}

uint32_t lv_port_tick_get(void)
{
    return clock()/1000;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

