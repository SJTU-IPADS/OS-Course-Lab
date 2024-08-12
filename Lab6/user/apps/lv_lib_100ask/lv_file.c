#include "lv_lib_100ask.h"
#include "test/lv_100ask_file_explorer/simple_test.h"
#include <unistd.h>
#include <stdio.h>
#include "lv_conf.h"
#include "lv_drivers/wayland/wayland.h"
#include <time.h>
#include <stdlib.h>

#define H_RES (800)
#define V_RES (600)
lv_disp_t * disp;

extern lv_obj_t * file_list;
void (*scroll_old_read_cb)(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

static void scroll_read_cb(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data){
   scroll_old_read_cb(indev_drv, data);
   lv_obj_scroll_by(file_list, 0, data->enc_diff * 50, LV_ANIM_ON);
   data->enc_diff = 0;
}


int main(void)
{    
    lv_init();
    lv_wayland_init();
    disp = lv_wayland_create_window(H_RES, V_RES, "Window Title", NULL);
    lv_disp_set_default(disp);
    lv_group_t * g = lv_group_create();
    lv_group_set_default(g);
    lv_indev_set_group(lv_wayland_get_keyboard(disp), g);
    lv_indev_set_group(lv_wayland_get_pointer(disp), g);
    lv_indev_set_group(lv_wayland_get_pointeraxis(disp), g);
    scroll_old_read_cb = lv_wayland_get_pointeraxis(disp)->driver->read_cb;
    lv_wayland_get_pointeraxis(disp)->driver->read_cb = scroll_read_cb;
    lv_100ask_file_explorer_simple_test();
    while(lv_wayland_window_is_open(disp)){
        usleep(1000 * lv_wayland_timer_handler());                 //! run lv task at the max speed
    }
    lv_wayland_deinit();
    return 0;
}