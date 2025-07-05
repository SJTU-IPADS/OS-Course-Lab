
/**
 * @file simple_test.c
 *
 */



/*********************
 *      INCLUDES
 *********************/
#include "simple_test.h"

#if LV_100ASK_SKETCHPAD_SIMPLE_TEST != 0

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

void lv_100ask_sketchpad_simple_test(void)
{
    static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(SKETCHPAD_DEFAULT_WIDTH, SKETCHPAD_DEFAULT_HEIGHT)];
	lv_obj_t * sketchpad = lv_100ask_sketchpad_create(lv_scr_act());

	lv_canvas_set_buffer(sketchpad, cbuf, SKETCHPAD_DEFAULT_WIDTH, SKETCHPAD_DEFAULT_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    lv_obj_center(sketchpad);
    lv_canvas_fill_bg(sketchpad, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_OPA_COVER);
}


/*=====================
 * Other functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/


#endif  /*SIMPLE_TEST*/
