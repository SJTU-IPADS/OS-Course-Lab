
/**
 * @file simple_test.c
 *
 */



/*********************
 *      INCLUDES
 *********************/
#include "simple_test.h"

#if LV_100ASK_MEMORY_GAME_SIMPLE_TEST != 0

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
static lv_obj_t * level_label;

/**********************
 *      MACROS
 **********************/


/**********************
 *   GLOBAL FUNCTIONS
 **********************/
static void slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    lv_obj_t * memory_game = lv_event_get_user_data(e);

    lv_100ask_memory_game_set_map(memory_game, lv_slider_get_value(slider), lv_slider_get_value(slider));
    lv_label_set_text_fmt(level_label, "%d*%d", lv_100ask_memory_game_get_row(memory_game), lv_100ask_memory_game_get_row(memory_game));
}

void lv_100ask_memory_game_simple_test(void)
{
	lv_obj_t * memory_game = lv_100ask_memory_game_create(lv_scr_act());
    lv_obj_set_size(memory_game, 480, 480);
    //lv_100ask_memory_set_map(memory_game, 2, 2);
    lv_obj_set_style_text_font(memory_game, &lv_font_montserrat_30, 0);
    lv_obj_center(memory_game);

    lv_obj_t * slider = lv_slider_create(lv_scr_act());
    lv_obj_align_to(slider, memory_game, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_slider_set_range(slider, 2, 10);
    lv_slider_set_value(slider, lv_100ask_memory_game_get_row(memory_game), LV_ANIM_ON);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, memory_game);

    level_label = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(level_label, "%d*%d", lv_100ask_memory_game_get_row(memory_game), lv_100ask_memory_game_get_row(memory_game));

    lv_obj_align_to(level_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
}


/*=====================
 * Other functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/


#endif  /*LV_100ASK_MEMORY_GAME_SIMPLE_TEST*/
