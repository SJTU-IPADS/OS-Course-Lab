
/**
 * @file simple_test.c
 *
 */



/*********************
 *      INCLUDES
 *********************/
#include "lv_100ask_nes_simple_test.h"

#if LV_100ASK_NES_SIMPLE_TEST != 0

#if LV_100ASK_NES_PLATFORM_POSIX
    #include <pthread.h>
#elif LV_100ASK_NES_PLATFORM_FREERTOS
    /* Scheduler include files. */
    #include "FreeRTOSConfig.h"
    #include "FreeRTOS.h"
    #include "task.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/
static void init_my_nes_front_end(lv_obj_t * parent);
static void ctrl_btnm_event_cb(lv_event_t * e);
static void file_explorer_event_cb(lv_event_t * e);
static void menu_scroll_event_cb(lv_event_t * e);
static void slider_event_cb(lv_event_t * e);

static void layer_top_event_cb(lv_event_t * e);

static lv_obj_t * create_text(lv_obj_t * parent, const char * icon, const char * txt,
                                        lv_menu_builder_variant_t builder_variant);
static lv_obj_t * create_slider(lv_obj_t * parent,
                                   const char * icon, const char * txt, int32_t min, int32_t max, int32_t val);

/**********************
 *  STATIC VARIABLES
 **********************/

// ctrl btnm
static const char * btnm_dir_map[] = {"1", LV_SYMBOL_UP, "3", "\n",
                                    LV_SYMBOL_LEFT, "5", LV_SYMBOL_RIGHT, "\n",
                                    "4", LV_SYMBOL_DOWN, "6", ""};

static const char * btnm_menu_map[] = {"SELECT", LV_SYMBOL_LIST, "START", ""};
static const char * btnm_opt_map[] = {"A", "B", "3",""};

/**********************
 *      MACROS
 **********************/


/**********************
 *   GLOBAL FUNCTIONS
 **********************/
lv_obj_t * my_nes;

void lv_100ask_nes_simple_test(void)
{
    my_nes = lv_100ask_nes_create(lv_scr_act());
    lv_obj_center(my_nes);

    init_my_nes_front_end(my_nes);

#if LV_100ASK_NES_PLATFORM_POSIX
    pthread_t tid1;
    int ret = pthread_create(&tid1, NULL, &lv_100ask_nes_run, (void *)my_nes);//创建线程传入变量a的地址
    if(ret != 0) {
        LV_LOG_ERROR("Pthread create error!");
        return;
    }
#elif LV_100ASK_NES_PLATFORM_FREERTOS
    taskENTER_CRITICAL();
    xTaskCreate(lv_100ask_nes_run, "lv_100ask_nes_run", 2048, (void *)my_nes, (tskIDLE_PRIORITY + 3), (TaskHandle_t *) NULL );
    printf("Free heap: %d bytes\n\r", xPortGetFreeHeapSize());
	taskEXIT_CRITICAL();
#endif

}


/*=====================
 * Other functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void init_my_nes_front_end(lv_obj_t * parent)
{
    /*ctrl btnm style*/
    static lv_style_t nes_ctrl_btn_style;
    lv_style_init(&nes_ctrl_btn_style);
    lv_style_set_bg_opa(&nes_ctrl_btn_style, LV_OPA_0);
    lv_style_set_pad_all(&nes_ctrl_btn_style, 4);
    lv_style_set_radius(&nes_ctrl_btn_style, 0);
    lv_style_set_border_width(&nes_ctrl_btn_style, 0);

    lv_obj_t * ctrl_area = lv_obj_create(parent);
    lv_obj_remove_style_all(ctrl_area);
    lv_obj_set_size(ctrl_area, LV_PCT(100), LV_PCT(50));
    lv_obj_align(ctrl_area,LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t * btnm_dir = lv_btnmatrix_create(ctrl_area);
    lv_obj_set_height(btnm_dir, LV_PCT(60));
    lv_obj_align(btnm_dir, LV_ALIGN_LEFT_MID, 0, 0);
    lv_btnmatrix_set_map(btnm_dir, btnm_dir_map);
    lv_btnmatrix_set_btn_ctrl_all(btnm_dir, LV_BTNMATRIX_CTRL_CHECKED);
    lv_btnmatrix_set_btn_ctrl(btnm_dir, 0, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_btnmatrix_set_btn_ctrl(btnm_dir, 2, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_btnmatrix_set_btn_ctrl(btnm_dir, 4, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_btnmatrix_set_btn_ctrl(btnm_dir, 6, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_btnmatrix_set_btn_ctrl(btnm_dir, 8, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_obj_add_style(btnm_dir, &nes_ctrl_btn_style, 0);

    lv_obj_t * btnm_menu = lv_btnmatrix_create(ctrl_area);
    lv_obj_set_height(btnm_menu, LV_PCT(10));
    lv_obj_align(btnm_menu, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_btnmatrix_set_map(btnm_menu, btnm_menu_map);
    lv_btnmatrix_set_btn_ctrl_all(btnm_menu, LV_BTNMATRIX_CTRL_CHECKED);
    //lv_btnmatrix_set_btn_ctrl(btnm_menu, 1, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_btn_width(btnm_menu, 0, 3);
    lv_btnmatrix_set_btn_width(btnm_menu, 1, 1);
    lv_btnmatrix_set_btn_width(btnm_menu, 2, 3);
    lv_obj_add_style(btnm_menu, &nes_ctrl_btn_style, 0);

    lv_obj_t * btnm_opt = lv_btnmatrix_create(ctrl_area);
    //lv_obj_set_height(btnm_opt, LV_PCT(20));
    lv_obj_set_size(btnm_opt, LV_PCT(25), LV_PCT(20));
    lv_obj_align(btnm_opt, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_btnmatrix_set_map(btnm_opt, btnm_opt_map);
    lv_btnmatrix_set_btn_ctrl_all(btnm_opt, LV_BTNMATRIX_CTRL_CHECKED);
    lv_btnmatrix_set_btn_ctrl(btnm_opt, 2, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_obj_set_style_radius(btnm_opt, 360, LV_PART_ITEMS);
    lv_obj_add_style(btnm_opt, &nes_ctrl_btn_style, 0);

    lv_obj_add_event_cb(btnm_dir, ctrl_btnm_event_cb, LV_EVENT_ALL, parent);
    lv_obj_add_event_cb(btnm_menu, ctrl_btnm_event_cb, LV_EVENT_ALL, parent);
    lv_obj_add_event_cb(btnm_opt, ctrl_btnm_event_cb, LV_EVENT_ALL, parent);

    /*menu*/
    lv_obj_t * menu_area = lv_tabview_create(lv_layer_top(), LV_DIR_TOP, 0);
    lv_obj_center(menu_area);
    lv_obj_set_size(menu_area, LV_PCT(70), LV_PCT(70));
    lv_obj_set_style_radius(menu_area, 8, LV_PART_MAIN);

    //Add some tabs (the tabs are page (lv_page) and can be scrolled
    lv_obj_t * tab1 = lv_tabview_add_tab(menu_area, "Setting");
    lv_obj_t * tab2 = lv_tabview_add_tab(menu_area, "File explorer");
    lv_tabview_set_act(menu_area, 1, LV_ANIM_OFF);
    lv_obj_add_event_cb(lv_tabview_get_content(menu_area), menu_scroll_event_cb, LV_EVENT_SCROLL_END, NULL);

    // tab 1: setting
    lv_obj_set_flex_flow(tab1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab1, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    create_slider(tab1, LV_SYMBOL_SETTINGS, "Volume", 0, 100, 0);
    create_slider(tab1, LV_SYMBOL_SETTINGS, "Brightness", 0, 100, 80);
    create_slider(tab1, LV_SYMBOL_SETTINGS, "Zoom", 0, 1024, 256); // <256: scale down, >256 scale up, 128 half size, 512 double size
    create_slider(tab1, LV_SYMBOL_SETTINGS, "Velocity", 0, 50000, 0);  // max 100ms(usleep)

    // tab 2: files explorer
    lv_obj_t * file_explorer = lv_100ask_file_explorer_create(tab2);
    lv_obj_center(file_explorer);
    //lv_obj_align(file_explorer, LV_ALIGN_TOP_RIGHT, 0 ,0);
    lv_obj_set_size(file_explorer, LV_PCT(100), LV_PCT(100));

#if LV_USE_FS_WIN32
    lv_100ask_file_explorer_open_dir(file_explorer, "D:/ROM/NES");
#else
    lv_100ask_file_explorer_open_dir(file_explorer, "/root/ROM/NES");
#endif

    lv_obj_add_event_cb(file_explorer, file_explorer_event_cb, LV_EVENT_VALUE_CHANGED, parent);

    /* Modal Dialogue Box */
    lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_50, 0);
    lv_obj_set_style_bg_color(lv_layer_top(), lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(lv_layer_top(), layer_top_event_cb, LV_EVENT_CLICKED, parent);

    /* 页面切换提示(Page switching prompt) */
    lv_obj_t * label;
    label = lv_label_create(lv_layer_top());
    lv_label_set_text(label, LV_SYMBOL_LEFT"  ");
    lv_obj_align_to(label, menu_area, LV_ALIGN_OUT_LEFT_MID, 0, 0);

    label = lv_label_create(lv_layer_top());
    lv_label_set_text(label, "  "LV_SYMBOL_RIGHT);
    lv_obj_align_to(label, menu_area, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    lv_100ask_nes_set_state(parent, LV_100ASK_NES_STATE_MENU);
    lv_100ask_nes_set_key(parent, LV_100ASK_NES_KEY_MENU, LV_100ASK_NES_KEY_STATE_RELEASED);
    lv_100ask_nes_set_lock(parent);
}

static void ctrl_btnm_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btnm = lv_event_get_target(e);
    lv_obj_t * nes = lv_event_get_user_data(e);

    /*KEY DOWN*/
    if(code == LV_EVENT_VALUE_CHANGED) {
        uint32_t id = lv_btnmatrix_get_selected_btn(btnm);
        const char * txt = lv_btnmatrix_get_btn_text(btnm, id);

        if(strcmp(txt, LV_SYMBOL_UP) == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_UP, LV_100ASK_NES_KEY_STATE_PRESSED);
        else if(strcmp(txt, LV_SYMBOL_DOWN) == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_DOWN, LV_100ASK_NES_KEY_STATE_PRESSED);
        else if(strcmp(txt, LV_SYMBOL_LEFT) == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_LEFT, LV_100ASK_NES_KEY_STATE_PRESSED);
        else if(strcmp(txt, LV_SYMBOL_RIGHT) == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_RIGHT, LV_100ASK_NES_KEY_STATE_PRESSED);
        else if(strcmp(txt, "A") == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_A, LV_100ASK_NES_KEY_STATE_PRESSED);
        else if(strcmp(txt, "B") == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_B, LV_100ASK_NES_KEY_STATE_PRESSED);
        else if(strcmp(txt, "START") == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_START, LV_100ASK_NES_KEY_STATE_PRESSED);
        else if(strcmp(txt, "SELECT") == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_SELECT, LV_100ASK_NES_KEY_STATE_PRESSED);

        //LV_LOG_USER("KEY DOWN");
    }
    /*KEY UP*///dwPad1 &= ~(1<<0);
    else if(code == LV_EVENT_RELEASED) {
        uint32_t id = lv_btnmatrix_get_selected_btn(btnm);
        const char * txt = lv_btnmatrix_get_btn_text(btnm, id);

        if(strcmp(txt, LV_SYMBOL_UP) == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_UP, LV_100ASK_NES_KEY_STATE_RELEASED);
        else if(strcmp(txt, LV_SYMBOL_DOWN) == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_DOWN, LV_100ASK_NES_KEY_STATE_RELEASED);
        else if(strcmp(txt, LV_SYMBOL_LEFT) == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_LEFT, LV_100ASK_NES_KEY_STATE_RELEASED);
        else if(strcmp(txt, LV_SYMBOL_RIGHT) == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_RIGHT, LV_100ASK_NES_KEY_STATE_RELEASED);
        else if(strcmp(txt, "A") == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_A, LV_100ASK_NES_KEY_STATE_RELEASED);
        else if(strcmp(txt, "B") == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_B, LV_100ASK_NES_KEY_STATE_RELEASED);
        else if(strcmp(txt, "START") == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_START, LV_100ASK_NES_KEY_STATE_RELEASED);
        else if(strcmp(txt, "SELECT") == 0)
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_SELECT, LV_100ASK_NES_KEY_STATE_RELEASED);
        else if(strcmp(txt, LV_SYMBOL_LIST) == 0) {
            lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
            lv_obj_clear_flag(lv_layer_top(), LV_OBJ_FLAG_HIDDEN);
            lv_100ask_nes_set_key(nes, LV_100ASK_NES_KEY_MENU, LV_100ASK_NES_KEY_STATE_RELEASED);
        }
        //LV_LOG_USER("KE UP");
    }
}


static void file_explorer_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * file_explorer = lv_event_get_target(e);
    lv_obj_t * nes = lv_event_get_user_data(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        char * path = lv_100ask_file_explorer_get_cur_path(file_explorer);
        char * fn = lv_100ask_file_explorer_get_sel_fn(file_explorer);
        uint16_t path_len = strlen(path);
        uint16_t fn_len = strlen(fn);

        if ((path_len + fn_len) <= LV_100ASK_FILE_EXPLORER_PATH_MAX_LEN) {
            char sel_fn[LV_100ASK_FILE_EXPLORER_PATH_MAX_LEN];

            strcpy(sel_fn, path);
            if(*(path + path_len) != '/')
                strcat(sel_fn, "/");
            strcat(sel_fn, fn);

            lv_100ask_nes_set_state(nes, LV_100ASK_NES_STATE_NEW_GAME);
            lv_100ask_nes_set_fn(nes, sel_fn);
            lv_event_send(lv_layer_top(), LV_EVENT_CLICKED, NULL);
            lv_100ask_nes_set_unlock(nes);

            LV_LOG_USER("%s", sel_fn);
        }
    }
}

// 处理滚动事件，完成页面循环切换(无限切换)
// Handle scrolling events and complete page cycle switching (infinite switching)
static void menu_scroll_event_cb(lv_event_t * e)
{
    lv_obj_t * cont = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    lv_obj_t * tv = lv_obj_get_parent(cont);

    if(lv_event_get_code(e) == LV_EVENT_SCROLL_END) {
        lv_tabview_t * tabview = (lv_tabview_t *)tv;

        lv_coord_t s = lv_obj_get_scroll_x(cont);

        lv_point_t p;
        lv_obj_get_scroll_end(cont, &p);

        lv_coord_t w = lv_obj_get_content_width(cont);
        lv_coord_t t;

        if(lv_obj_get_style_base_dir(tv, LV_PART_MAIN) == LV_BASE_DIR_RTL)  t = -(p.x - w / 2) / w;
        else t = (p.x + w / 2) / w;

        if(s < 0) t = tabview->tab_cnt - 1;
        else if((t == (tabview->tab_cnt - 1)) && (s > p.x)) t = 0;

        bool new_tab = false;
        if(t != lv_tabview_get_tab_act(tv)) new_tab = true;
        lv_tabview_set_act(tv, t, LV_ANIM_OFF);
    }
}

static void layer_top_event_cb(lv_event_t * e)
{
    lv_obj_t * layer_top = lv_event_get_target(e);
    lv_obj_t * nes = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);


    if ((code == LV_EVENT_CLICKED))
    {
        if(lv_100ask_nes_get_fn(nes) != NULL) {
            lv_obj_add_flag(layer_top, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(layer_top, LV_OBJ_FLAG_CLICKABLE);   // 清除标志(lv_layer_top层)
            lv_100ask_nes_set_unlock(nes);
        }
    }
}


static lv_obj_t * create_text(lv_obj_t * parent, const char * icon, const char * txt,
                                        lv_menu_builder_variant_t builder_variant)
{
    lv_obj_t * obj = lv_menu_cont_create(parent);

    lv_obj_t * img = NULL;
    lv_obj_t * label = NULL;

    if(icon) {
        img = lv_img_create(obj);
        lv_img_set_src(img, icon);
    }

    if(txt) {
        label = lv_label_create(obj);
        lv_label_set_text(label, txt);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(label, 1);
    }

    if(builder_variant == LV_MENU_ITEM_BUILDER_VARIANT_2 && icon && txt) {
        lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
        lv_obj_swap(img, label);
    }

    return obj;
}

static lv_obj_t * create_slider(lv_obj_t * parent, const char * icon, const char * txt, int32_t min, int32_t max, int32_t val)
{
    lv_obj_t * obj = create_text(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_2);

    lv_obj_t * slider = lv_slider_create(obj);
    lv_obj_set_flex_grow(slider, 1);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, val, LV_ANIM_OFF);

    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, obj);

    if(icon == NULL) {
        lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    }

    return obj;
}

static void slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    lv_obj_t * parent = lv_event_get_user_data(e);

    lv_obj_t * label = lv_obj_get_child(parent, 0);

    char * txt = lv_label_get_text(label);
    if(strcmp(txt, "Volume") == 0) {
         /*Do something*/
        return;
    }
    else if(strcmp(txt, "Brightness") == 0) {
        /*Do something*/
        return;
    }
    else if(strcmp(txt, "Zoom") == 0) {
        lv_100ask_nes_set_zoom(my_nes, lv_slider_get_value(slider));
    }
    else if(strcmp(txt, "Velocity") == 0) {
        lv_100ask_nes_set_speed(my_nes, lv_slider_get_value(slider));
    }
}

#endif  /*SIMPLE_TEST*/
