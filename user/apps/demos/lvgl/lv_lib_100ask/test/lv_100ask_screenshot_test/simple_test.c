
/**
 * @file simple_test.c
 *
 */



/*********************
 *      INCLUDES
 *********************/
#include "simple_test.h"

#if LV_USE_100ASK_SCREENSHOT_TEST != 0
#include <time.h>

/*********************
 *      DEFINES
 *********************/
#if _WIN32 || WIN32
    #define LV_SCREENSHOT_SAVE_PATH     "D:/"
#else
    #define LV_SCREENSHOT_SAVE_PATH     "//"
#endif

#define BLINK_TIME                  300

#define SCREENSHOT_SAVE_AS_BMP      "bmp"
#define SCREENSHOT_SAVE_AS_PNG      "png"
#define SCREENSHOT_SAVE_AS_JPG      "jpg"

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
static void blink_timer(lv_timer_t * timer)
{
    lv_obj_t * obj_blink = (lv_obj_t *)timer->user_data;

    lv_obj_add_flag(obj_blink, LV_OBJ_FLAG_HIDDEN);
}

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_t * obj_blink = lv_event_get_user_data(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        char dropdown_selected_str_buf[16];
        char file_name_buf[128];
        time_t timep;
        struct tm *p;
        char time_buffer [64];

        lv_dropdown_get_selected_str(obj, dropdown_selected_str_buf, sizeof(dropdown_selected_str_buf));

        time (&timep);
        p=gmtime(&timep);
        strftime (time_buffer, sizeof(time_buffer),"lv_100ask_screenshot-%Y%m%d-%H%M%S",p);

        lv_snprintf(file_name_buf, sizeof(file_name_buf), "%s%s.%s", LV_SCREENSHOT_SAVE_PATH, time_buffer, dropdown_selected_str_buf);
        LV_LOG_USER("New screenshot name: %s", file_name_buf);

        if(strcmp(dropdown_selected_str_buf, SCREENSHOT_SAVE_AS_BMP) == 0)
            lv_100ask_screenshot_create(lv_scr_act(), LV_IMG_CF_TRUE_COLOR_ALPHA, LV_100ASK_SCREENSHOT_SV_BMP, file_name_buf);
        else if(strcmp(dropdown_selected_str_buf, SCREENSHOT_SAVE_AS_PNG) == 0)
            lv_100ask_screenshot_create(lv_scr_act(), LV_IMG_CF_TRUE_COLOR_ALPHA, LV_100ASK_SCREENSHOT_SV_PNG, file_name_buf);
        else if(strcmp(dropdown_selected_str_buf, SCREENSHOT_SAVE_AS_JPG) == 0)
            lv_100ask_screenshot_create(lv_scr_act(), LV_IMG_CF_TRUE_COLOR_ALPHA, LV_100ASK_SCREENSHOT_SV_JPEG, file_name_buf);
        else return;

        lv_obj_clear_flag(obj_blink, LV_OBJ_FLAG_HIDDEN);
        lv_timer_t * timer = lv_timer_create(blink_timer, BLINK_TIME, obj_blink);
        lv_timer_set_repeat_count(timer, 1);
    }
}


void lv_100ask_screenshot_test(void)
{
#if 0
    lv_obj_t * obj = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(obj);

    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(obj, LV_OPA_100 ,0);

    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0000), 0);
    //lv_obj_set_style_bg_color(obj, lv_palette_main(LV_PALETTE_BLUE) ,0);
#endif

    /*Create a normal drop down list*/
    lv_obj_t * dd = lv_dropdown_create(lv_scr_act());
    lv_dropdown_set_options(dd, SCREENSHOT_SAVE_AS_BMP"\n"
                                SCREENSHOT_SAVE_AS_PNG"\n"
                                SCREENSHOT_SAVE_AS_JPG);

    lv_obj_align(dd, LV_ALIGN_TOP_RIGHT, -10, 10);


    /*Blinking effect*/
    lv_obj_t * obj_blink = lv_obj_create(lv_scr_act());
    lv_obj_set_style_border_width(obj_blink, 0, 0);
    lv_obj_set_style_pad_all(obj_blink, 0, 0);
    lv_obj_set_style_radius(obj_blink, 0, 0);
    lv_obj_set_size(obj_blink, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(obj_blink, lv_color_hex(0x000000), 0);
    lv_obj_add_flag(obj_blink, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(dd, event_handler, LV_EVENT_ALL, obj_blink);
}


/*=====================
 * Other functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/


#endif  /*SIMPLE_TEST*/
