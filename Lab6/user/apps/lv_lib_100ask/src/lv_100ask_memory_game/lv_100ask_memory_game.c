/**
 * @file lv_100ask_memory_game.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_100ask_memory_game.h"

#if LV_USE_100ASK_MEMORY_GAME != 0

#include<time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_100ask_memory_game_class
/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_100ask_memory_game_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_100ask_memory_game_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_100ask_memory_game_event(const lv_obj_class_t * class_p, lv_event_t * e);

static void list_rand_number(uint16_t arry[], uint16_t max_count, uint16_t count);
static void item_event_handler(lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_100ask_memory_game_class = {
    .constructor_cb = lv_100ask_memory_game_constructor,
    .destructor_cb  = lv_100ask_memory_game_destructor,
    .event_cb       = lv_100ask_memory_game_event,
    .width_def      = LV_DPI_DEF * 2,
    .height_def     = LV_DPI_DEF * 2,
    .instance_size  = sizeof(lv_100ask_memory_game_t),
    .base_class     = &lv_obj_class
};

static lv_style_t cont_style;
static lv_style_t item_def_style;
static lv_style_t item_click_style;
static lv_style_t item_hit_style;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_100ask_memory_game_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}



/*=====================
 * Setter functions
 *====================*/

void lv_100ask_memory_game_set_map(lv_obj_t * obj, uint16_t row, uint16_t col)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    if ((row == 0) || (col == 0))   return;
    if ((row % 2)) row++;
    if ((col % 2)) col++;
    
    lv_100ask_memory_game_t * memory_game = (lv_100ask_memory_game_t *)obj;

    int16_t count = memory_game->row * memory_game->col;
    int16_t cur_count = row * col;
    int16_t new_count = 0;

    new_count = count - cur_count;
    if(new_count > 0)
    {   
        lv_obj_t * item;
        for(int16_t i = 0; i < new_count; i++)
        {
            /*Delete the hit object first*/
            for(uint16_t i = 0; i < lv_obj_get_child_cnt(obj); i++) {
                lv_obj_t * child = lv_obj_get_child(obj, i);
                if((child) && (!lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE)))
                {
                    lv_obj_del(child);
                    break;
                }
                else if ((i+1) == lv_obj_get_child_cnt(obj))
                {
                    lv_obj_del(child);
                }
            }
        }
    }
    else if(new_count < 0)
    {
        lv_obj_t * item;
        lv_obj_t * label;
        for(int16_t i = 0; i > new_count; i--)
        {
            item = lv_btn_create(obj);
            lv_obj_add_style(item, &item_def_style, 0);
            lv_obj_add_style(item, &item_click_style, LV_STATE_PRESSED);

            label = lv_label_create(item);
            lv_obj_center(label);
        }
        /*Recover the hit object*/
        for(uint16_t i = 0; i < lv_obj_get_child_cnt(obj); i++) {
            lv_obj_t * child = lv_obj_get_child(obj, i);
            if((child) && (!lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE)))
            {
                lv_obj_remove_style(child, &item_hit_style, 0);
                lv_obj_add_flag(child, LV_OBJ_FLAG_CLICKABLE);
            }
        }
    }

    uint16_t list_number[cur_count + 1];
    list_rand_number(list_number, cur_count, (cur_count / 2)); 

    lv_coord_t w = lv_obj_get_width(obj);
    lv_coord_t h = lv_obj_get_height(obj);
    for(uint16_t i = 0; i < lv_obj_get_child_cnt(obj); i++) {
        lv_obj_t * child = lv_obj_get_child(obj, i);
        if(child)
        {
            lv_obj_set_size(child, (w / row), (h / col));
            lv_obj_add_event_cb(child, item_event_handler, LV_EVENT_CLICKED, obj);
        }

        lv_obj_t * label = lv_obj_get_child(child, 0);
        if (label && i <= cur_count)
        {
            lv_label_set_text_fmt(label, "%d", list_number[i]);
            lv_obj_center(label);
        }
    }

    memory_game->bef = NULL;
    memory_game->row = row;
    memory_game->col = col;

    lv_obj_invalidate(obj);
}



/*=====================
 * Getter functions
 *====================*/
/**
 * Get row.
 * @param obj   pointer to a memory game object
 * @return      row
 */
uint16_t lv_100ask_memory_game_get_row(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    
    lv_100ask_memory_game_t * memory_game = (lv_100ask_memory_game_t *)obj;

    return memory_game->row;
}

/**
 * Get col.
 * @param obj   pointer to a memory game object
 * @return      col
 */
uint16_t lv_100ask_memory_game_get_col(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    
    lv_100ask_memory_game_t * memory_game = (lv_100ask_memory_game_t *)obj;

    return memory_game->col;
}


/*=====================
 * Other functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_100ask_memory_game_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_100ask_memory_game_t * memory_game = (lv_100ask_memory_game_t *)obj;

    memory_game->bef = NULL;
    memory_game->row = 0;
    memory_game->col = 0;

    lv_style_init(&cont_style);
    lv_style_set_flex_flow(&cont_style, LV_FLEX_FLOW_ROW_WRAP);
    lv_style_set_flex_main_place(&cont_style, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_style_set_layout(&cont_style, LV_LAYOUT_FLEX);
    lv_style_set_pad_all(&cont_style, 0);

	lv_style_init(&item_def_style);
	lv_style_set_radius(&item_def_style, 0);
    lv_style_set_border_width(&item_def_style, 1);
    lv_style_set_border_color(&item_def_style, lv_color_hex(0xffffff));
    lv_style_set_text_opa(&item_def_style, LV_OPA_0);
    lv_style_set_shadow_opa(&item_def_style, LV_OPA_0);

    lv_style_init(&item_hit_style);
    lv_style_set_border_width(&item_hit_style, 0);
    lv_style_set_bg_opa(&item_hit_style, LV_OPA_0);
    lv_style_set_text_opa(&item_hit_style, LV_OPA_0);
    lv_style_set_border_opa(&item_hit_style, LV_OPA_0);
    lv_style_set_shadow_opa(&item_hit_style, LV_OPA_0);

    lv_style_init(&item_click_style);
    lv_style_set_border_width(&item_click_style, 0);
    lv_style_set_text_opa(&item_click_style, LV_OPA_100);
    lv_style_set_bg_opa(&item_click_style, LV_OPA_100);
    lv_style_set_border_opa(&item_click_style, LV_OPA_100);
    lv_style_set_shadow_opa(&item_click_style, LV_OPA_100);

    lv_obj_add_style(obj, &cont_style, 0);

    lv_obj_update_layout(obj);
    lv_100ask_memory_game_set_map(obj, LV_100ASK_MEMORY_GAME_DEFAULT_ROW, LV_100ASK_MEMORY_GAME_DEFAULT_COL);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_100ask_memory_game_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
}


static void lv_100ask_memory_game_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_res_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RES_OK) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    lv_100ask_memory_game_t * memory_game = (lv_100ask_memory_game_t *)obj;

    if (code == LV_EVENT_SIZE_CHANGED)
    {
        lv_100ask_memory_game_set_map(obj, memory_game->row, memory_game->col);
    }
}



static void item_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_t * user_data = lv_event_get_user_data(e);

    lv_100ask_memory_game_t * memory_game = (lv_100ask_memory_game_t *)user_data;

    if(code == LV_EVENT_CLICKED)
    {
        if (NULL == memory_game->bef)
        {
            memory_game->bef = lv_event_get_target(e);
            return;
        }
        else
        {
            if(obj == memory_game->bef) return;
        }

        if (strcmp(\
                   lv_label_get_text(lv_obj_get_child(obj, 0)),\
                   lv_label_get_text(lv_obj_get_child(memory_game->bef, 0))) == 0)
        {
            lv_obj_add_style(obj, &item_hit_style, 0);
            lv_obj_add_style(memory_game->bef, &item_hit_style, 0);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_clear_flag(memory_game->bef, LV_OBJ_FLAG_CLICKABLE);
        }

        memory_game->bef = obj;
    }
}



static void list_rand_number(uint16_t arry[], uint16_t max_count, uint16_t count)
{
	int w, t;

	srand((unsigned)time(NULL));

	for (int i = 0; i < max_count; i++)
	    arry[i] = (i % count) + 1;
	for (int i = 0; i < max_count; i++)
	{
		w = rand() % (count - (i % count)) + i;
        if (w > max_count)  w = max_count - 1;

	    t = arry[i];
	    arry[i] = arry[w];
	    arry[w] = t;
	}
}


#endif  /*LV_USE_100ASK_MEMORY_GAME*/
