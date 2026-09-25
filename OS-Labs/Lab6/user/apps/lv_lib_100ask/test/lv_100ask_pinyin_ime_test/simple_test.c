
/**
 * @file simple_test.c
 *
 */



/*********************
 *      INCLUDES
 *********************/
#include "simple_test.h"

#if LV_100ASK_PINYIN_IME_SIMPLE_TEST != 0

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

// Oops!!! You can extract Pinyin fonts Zip get it.
LV_FONT_DECLARE(lv_font_source_han_sans_normal_16);

static void ta_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = lv_event_get_user_data(e);

    if(code == LV_EVENT_FOCUSED) {
        if(lv_indev_get_type(lv_indev_get_act()) != LV_INDEV_TYPE_KEYPAD) {
            lv_keyboard_set_textarea(kb, ta);
            lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(ta, LV_STATE_FOCUSED);
        lv_indev_reset(NULL, ta);   /*To forget the last clicked object to make it focusable again*/
    }
}

void lv_100ask_pinyin_ime_simple_test(void)
{
    lv_obj_t * pinyin_ime = lv_100ask_pinyin_ime_create(lv_scr_act());
    lv_obj_set_style_text_font(pinyin_ime, &lv_font_source_han_sans_normal_16, 0);   // set font
    //lv_100ask_pinyin_ime_set_dict(pinyin_ime, your_dict); // Use a custom thesaurus. If it is not set, the built-in thesaurus will be used

    lv_obj_t * kb = lv_100ask_pinyin_ime_get_kb(pinyin_ime);

    /* ta1 */
    lv_obj_t * ta1 = lv_textarea_create(lv_scr_act());
    lv_obj_set_style_text_font(ta1, &lv_font_source_han_sans_normal_16, 0);   // set font
    lv_obj_align(ta1, LV_ALIGN_TOP_LEFT, 10, 10);

    lv_keyboard_set_textarea(kb, ta1);

    lv_obj_add_event_cb(ta1, ta_event_cb, LV_EVENT_ALL, kb);

    /* ta2 */
    lv_obj_t * ta2 = lv_textarea_create(lv_scr_act());
    lv_obj_set_style_text_font(ta2, &lv_font_source_han_sans_normal_16, 0);   // set font
    lv_obj_align_to(ta2, ta1,LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    lv_obj_add_event_cb(ta2, ta_event_cb, LV_EVENT_ALL, kb);
}


/*=====================
 * Other functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/


#endif  /*SIMPLE_TEST*/
