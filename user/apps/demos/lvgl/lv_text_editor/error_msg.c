#include <lvgl/lvgl.h>
#include <lv_drivers/wayland/wayland.h>

/* 事件处理函数 */
static void event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        /* 获取消息框对象 */
        lv_obj_t * mbox = lv_event_get_user_data(e);

        /* 删除消息框 */
        lv_msgbox_close(mbox);
    }
}

static void close_event_handler_2(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_DELETE) {
        lv_wayland_close_window(lv_disp_get_default());
    }
}

/* 创建模态消息框 */
void create_modal_message_box(char* title, char* text, uint8_t close) {
    /* 创建消息框，将父对象设置为NULL使其成为模态 */
        static const char * btns[] = {"OK", ""}; // 按钮文本数组
    lv_obj_t * mbox = lv_msgbox_create(NULL, title, text, btns, true);
    lv_obj_align(mbox, LV_ALIGN_CENTER, 0, 0); // 居中对齐
    /* 为消息框的每个按钮添加事件处理函数 */
    lv_obj_t * btn1 = lv_msgbox_get_btns(mbox);
    lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_CLICKED, mbox);

    if(close){
        lv_obj_add_event_cb(mbox, close_event_handler_2, LV_EVENT_DELETE, NULL);
    }
}

