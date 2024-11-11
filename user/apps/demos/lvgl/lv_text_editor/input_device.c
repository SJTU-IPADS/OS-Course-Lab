#include <lvgl/lvgl.h>
#include <lv_drivers/wayland/wayland.h>

#include "input_device.h"
#include "gui_guider.h"

static lv_indev_t * kb;
static lv_obj_t * kb_ta;
static lv_indev_t * pointer;
struct {
    lv_indev_t* pointer_axis;
    void (*old_read_cb)(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
} pointer_axis;

extern lv_ui guider_ui;

static void scroll_read_cb(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data){
   pointer_axis.old_read_cb(indev_drv, data);
   lv_obj_t * focused_obj = lv_group_get_focused(lv_group_get_default());
   if(focused_obj != guider_ui.screen_ta_1) {
        return;
   }
   lv_obj_scroll_by(guider_ui.screen_ta_1, 0, data->enc_diff * 10, LV_ANIM_OFF);
   data->enc_diff = 0;
}

void init_input_device(void){
    lv_group_t * g = lv_group_create();
    lv_group_set_default(g);
    kb = lv_wayland_get_keyboard(lv_disp_get_default());
    pointer = lv_wayland_get_pointer(lv_disp_get_default());
    pointer_axis.pointer_axis = lv_wayland_get_pointeraxis(lv_disp_get_default());
    pointer_axis.old_read_cb = pointer_axis.pointer_axis->driver->read_cb;
    lv_indev_set_group(kb, g);
    lv_indev_set_group(pointer, g);
    lv_indev_set_group(pointer_axis.pointer_axis, g);
    pointer_axis.pointer_axis->driver->read_cb = scroll_read_cb;
    kb_ta = lv_keyboard_create(lv_scr_act());
}

void bind_input_device(void){
    lv_obj_add_flag(guider_ui.screen_ta_1, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(guider_ui.screen_ta_1, LV_OBJ_FLAG_SCROLL_WITH_ARROW);
    lv_keyboard_set_textarea(kb_ta, guider_ui.screen_ta_1);
}