
/**
 * @file simple_test.c
 *
 */



/*********************
 *      INCLUDES
 *********************/
#include "simple_test.h"

#if LV_100ASK_PAGE_MANAGER_SIMPLE_TEST != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/



/**********************
 *  STATIC PROTOTYPES
 **********************/
static void init_main_page(lv_obj_t * page);
static void init_page1(lv_obj_t * page);
static void init_page2(lv_obj_t * page);
static void init_page3(lv_obj_t * page);
static void init_page4(lv_obj_t * page);
static void init_page5(lv_obj_t * page);
static void init_page6(lv_obj_t * page);
static void open_page_anim(lv_obj_t * obj);
static void close_page_anim(lv_obj_t * obj);
/**********************
 *  STATIC VARIABLES
 **********************/


/**********************
 *      MACROS
 **********************/


/**********************
 *   GLOBAL FUNCTIONS
 **********************/


void lv_100ask_page_manager_simple_test(void)
{
    lv_obj_t * page_manager = lv_100ask_page_manager_create(lv_scr_act());

    lv_obj_t * main_page = lv_100ask_page_manager_page_create(page_manager, "Main_page");
    lv_obj_t * page1 = lv_100ask_page_manager_page_create(page_manager, "Page1");
    lv_obj_t * page2 = lv_100ask_page_manager_page_create(page_manager, "Page2");
    lv_obj_t * page3 = lv_100ask_page_manager_page_create(page_manager, "Page3");
    lv_obj_t * page4 = lv_100ask_page_manager_page_create(page_manager, "Page4");
    lv_obj_t * page5 = lv_100ask_page_manager_page_create(page_manager, "Page5");
    lv_obj_t * page6 = lv_100ask_page_manager_page_create(page_manager, "Page6");

    lv_100ask_page_manager_set_page_init(main_page, init_main_page);
    lv_100ask_page_manager_set_page_init(page1, init_page1);
    lv_100ask_page_manager_set_page_init(page2, init_page2);
    lv_100ask_page_manager_set_page_init(page3, init_page3);
    lv_100ask_page_manager_set_page_init(page4, init_page4);
    lv_100ask_page_manager_set_page_init(page5, init_page5);
    lv_100ask_page_manager_set_page_init(page6, init_page6);

#if LV_100ASK_PAGE_MANAGER_COSTOM_ANIMARION
    lv_100ask_page_manager_set_open_page_anim(main_page, open_page_anim);
    lv_100ask_page_manager_set_close_page_anim(main_page, close_page_anim);
    lv_100ask_page_manager_set_open_page_anim(page1, open_page_anim);
    lv_100ask_page_manager_set_close_page_anim(page1, close_page_anim);
    lv_100ask_page_manager_set_open_page_anim(page2, open_page_anim);
    lv_100ask_page_manager_set_close_page_anim(page2, close_page_anim);
    lv_100ask_page_manager_set_open_page_anim(page3, open_page_anim);
    lv_100ask_page_manager_set_close_page_anim(page3, close_page_anim);
    lv_100ask_page_manager_set_open_page_anim(page4, open_page_anim);
    lv_100ask_page_manager_set_close_page_anim(page4, close_page_anim);
    lv_100ask_page_manager_set_open_page_anim(page5, open_page_anim);
    lv_100ask_page_manager_set_close_page_anim(page5, close_page_anim);
    lv_100ask_page_manager_set_open_page_anim(page6, open_page_anim);
    lv_100ask_page_manager_set_close_page_anim(page6, close_page_anim);
#endif

    lv_100ask_page_manager_set_main_page(page_manager, main_page);
    lv_100ask_page_manager_set_open_page(NULL, "Main_page");
}


/*=====================
 * Other functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/
#if LV_100ASK_PAGE_MANAGER_COSTOM_ANIMARION
/*open page anim*/
static void open_page_anim(lv_obj_t * obj)
{
    /*Do something with LVGL*/
    LV_LOG_USER("open page anim.");
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

/*close page anim*/
static void close_page_anim(lv_obj_t * obj)
{
    /*Do something with LVGL*/
    LV_LOG_USER("close page anim.");
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}
#endif

static void btn_event_handler(lv_event_t * e)
{
    lv_obj_t * page = (lv_obj_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        LV_LOG_USER("Clicked");
        lv_100ask_page_manager_open_previous_page(lv_obj_get_parent(page));
    }
}


/* main page */
static void init_main_page(lv_obj_t * page)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_flex_flow(&style, LV_FLEX_FLOW_ROW_WRAP);
    lv_style_set_flex_main_place(&style, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_style_set_layout(&style, LV_LAYOUT_FLEX);

    lv_obj_t * cont = lv_obj_create(page);
    lv_obj_set_size(cont, LV_PCT(95), LV_PCT(20));
    lv_obj_center(cont);
    lv_obj_add_style(cont, &style, 0);

    uint32_t i;
    for(i = 0; i < 6; i++) {
        lv_obj_t * obj = lv_btn_create(cont);
        lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_PCT(100));

        lv_obj_t * label = lv_label_create(obj);
        lv_label_set_text_fmt(label, "Page%d", i+1);
        lv_obj_center(label);

        lv_100ask_page_manager_set_load_page_event(obj, NULL, lv_label_get_text(label));
    }
}

/* page 1 */
static void init_page1(lv_obj_t * page)
{
    lv_obj_t * page_label = lv_label_create(page);
    lv_label_set_text(page_label, "You are browsing page 1");
    lv_obj_center(page_label);

    lv_obj_t * btn1 = lv_btn_create(page);
    lv_obj_set_size(btn1, 100, 50);
    lv_obj_align_to(btn1, page_label, LV_ALIGN_OUT_BOTTOM_MID, 80, 20);

    lv_obj_t * label1 = lv_label_create(btn1);
    lv_label_set_text(label1, "Open Page2");
    lv_obj_center(label1);

    lv_100ask_page_manager_set_load_page_event(btn1, NULL, "Page2");

    lv_obj_t * btn2 = lv_btn_create(page);
    lv_obj_set_size(btn2, 100, 50);
    lv_obj_align_to(btn2, page_label, LV_ALIGN_OUT_BOTTOM_MID, -80, 20);

    lv_obj_t * label2 = lv_label_create(btn2);
    lv_label_set_text(label2, "Open Page3");
    lv_obj_center(label2);

    lv_100ask_page_manager_set_load_page_event(btn2, NULL, "Page3");

    lv_obj_t * back_btn = lv_btn_create(page);
    lv_obj_add_event_cb(back_btn, btn_event_handler, LV_EVENT_CLICKED, page);  
}


/* page 2 */
static void init_page2(lv_obj_t * page)
{
    lv_obj_t * page_label = lv_label_create(page);
    lv_label_set_text(page_label, "You are browsing page 2");
    lv_obj_center(page_label);

    lv_obj_t * btn1 = lv_btn_create(page);
    lv_obj_set_size(btn1, 100, 50);
    lv_obj_align_to(btn1, page_label, LV_ALIGN_OUT_BOTTOM_MID, 80, 20);

    lv_obj_t * label1 = lv_label_create(btn1);
    lv_label_set_text(label1, "Open Page3");
    lv_obj_center(label1);

    lv_100ask_page_manager_set_load_page_event(btn1, NULL, "Page3");

    lv_obj_t * btn2 = lv_btn_create(page);
    lv_obj_set_size(btn2, 100, 50);
    lv_obj_align_to(btn2, page_label, LV_ALIGN_OUT_BOTTOM_MID, -80, 20);

    lv_obj_t * label2 = lv_label_create(btn2);
    lv_label_set_text(label2, "Open Page4");
    lv_obj_center(label2);

    lv_100ask_page_manager_set_load_page_event(btn2, NULL, "Page4");

    lv_obj_t * back_btn = lv_btn_create(page);
    lv_obj_add_event_cb(back_btn, btn_event_handler, LV_EVENT_CLICKED, page); 

}


/* page 3*/

static void init_page3(lv_obj_t * page)
{
    lv_obj_t * page_label = lv_label_create(page);
    lv_label_set_text(page_label, "You are browsing page 3");
    lv_obj_center(page_label);

    lv_obj_t * btn1 = lv_btn_create(page);
    lv_obj_set_size(btn1, 100, 50);
    lv_obj_align_to(btn1, page_label, LV_ALIGN_OUT_BOTTOM_MID, 80, 20);

    lv_obj_t * label1 = lv_label_create(btn1);
    lv_label_set_text(label1, "Open Page5");
    lv_obj_center(label1);

    lv_100ask_page_manager_set_load_page_event(btn1, NULL, "Page5");

    lv_obj_t * btn2 = lv_btn_create(page);
    lv_obj_set_size(btn2, 100, 50);
    lv_obj_align_to(btn2, page_label, LV_ALIGN_OUT_BOTTOM_MID, -80, 20);

    lv_obj_t * label2 = lv_label_create(btn2);
    lv_label_set_text(label2, "Open Page6");
    lv_obj_center(label2);

    lv_100ask_page_manager_set_load_page_event(btn2, NULL, "Page6");

    lv_obj_t * back_btn = lv_btn_create(page);
    lv_obj_add_event_cb(back_btn, btn_event_handler, LV_EVENT_CLICKED, page); 
}

/* page 4*/
static void init_page4(lv_obj_t * page)
{
    lv_obj_t * page_label = lv_label_create(page);
    lv_label_set_text(page_label, "You are browsing page 4");
    lv_obj_center(page_label);

    lv_obj_t * btn1 = lv_btn_create(page);
    lv_obj_set_size(btn1, 100, 50);
    lv_obj_align_to(btn1, page_label, LV_ALIGN_OUT_BOTTOM_MID, 80, 20);

    lv_obj_t * label1 = lv_label_create(btn1);
    lv_label_set_text(label1, "Open Page2");
    lv_obj_center(label1);

    lv_100ask_page_manager_set_load_page_event(btn1, NULL, "Page2");

    lv_obj_t * btn2 = lv_btn_create(page);
    lv_obj_set_size(btn2, 100, 50);
    lv_obj_align_to(btn2, page_label, LV_ALIGN_OUT_BOTTOM_MID, -80, 20);

    lv_obj_t * label2 = lv_label_create(btn2);
    lv_label_set_text(label2, "Open Page3");
    lv_obj_center(label2);

    lv_100ask_page_manager_set_load_page_event(btn2, NULL, "Page3");

    lv_obj_t * back_btn = lv_btn_create(page);
    lv_obj_add_event_cb(back_btn, btn_event_handler, LV_EVENT_CLICKED, page); 
}

/*page 5*/
static void init_page5(lv_obj_t * page)
{
    lv_obj_t * page_label = lv_label_create(page);
    lv_label_set_text(page_label, "You are browsing page 5");
    lv_obj_center(page_label);

    lv_obj_t * btn1 = lv_btn_create(page);
    lv_obj_set_size(btn1, 100, 50);
    lv_obj_align_to(btn1, page_label, LV_ALIGN_OUT_BOTTOM_MID, 80, 20);

    lv_obj_t * label1 = lv_label_create(btn1);
    lv_label_set_text(label1, "Open Page6");
    lv_obj_center(label1);

    lv_100ask_page_manager_set_load_page_event(btn1, NULL, "Page6");

    lv_obj_t * btn2 = lv_btn_create(page);
    lv_obj_set_size(btn2, 100, 50);
    lv_obj_align_to(btn2, page_label, LV_ALIGN_OUT_BOTTOM_MID, -80, 20);

    lv_obj_t * label2 = lv_label_create(btn2);
    lv_label_set_text(label2, "Open Page1");
    lv_obj_center(label2);

    lv_100ask_page_manager_set_load_page_event(btn2, NULL, "Page1");

    lv_obj_t * back_btn = lv_btn_create(page);
    lv_obj_add_event_cb(back_btn, btn_event_handler, LV_EVENT_CLICKED, page); 
}

/*page 6*/
static void init_page6(lv_obj_t * page)
{
    lv_obj_t * page_label = lv_label_create(page);
    lv_label_set_text(page_label, "You are browsing page 6");
    lv_obj_center(page_label);

    lv_obj_t * btn1 = lv_btn_create(page);
    lv_obj_set_size(btn1, 100, 50);
    lv_obj_align_to(btn1, page_label, LV_ALIGN_OUT_BOTTOM_MID, 80, 20);

    lv_obj_t * label1 = lv_label_create(btn1);
    lv_label_set_text(label1, "Open Page1");
    lv_obj_center(label1);

    lv_100ask_page_manager_set_load_page_event(btn1, NULL, "Page1");

    lv_obj_t * btn2 = lv_btn_create(page);
    lv_obj_set_size(btn2, 100, 50);
    lv_obj_align_to(btn2, page_label, LV_ALIGN_OUT_BOTTOM_MID, -80, 20);

    lv_obj_t * label2 = lv_label_create(btn2);
    lv_label_set_text(label2, "Open Page4");
    lv_obj_center(label2);

    lv_100ask_page_manager_set_load_page_event(btn2, NULL, "Page4");

    lv_obj_t * back_btn = lv_btn_create(page);
    lv_obj_add_event_cb(back_btn, btn_event_handler, LV_EVENT_CLICKED, page); 
}

#endif  /*SIMPLE_TEST*/
