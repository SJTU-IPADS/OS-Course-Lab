
/**
 * @file lv_100ask_page_manager.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_100ask_page_manager.h"


#if LV_USE_100ASK_PAGE_MANAGER != 0


/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_100ask_page_manager_class
#define MY_PAGE_CLASS &lv_100ask_page_manager_page_class

/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_100ask_page_manager_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_100ask_page_manager_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_100ask_page_manager_event(const lv_obj_class_t * class_p, lv_event_t * e);

static void lv_100ask_page_manager_page_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_100ask_page_manager_page_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_page_manager_load_page_event_cb(lv_event_t * e);
static void lv_100ask_page_manager_page_del_event_cb(lv_event_t * e);

static void page_close_anim_timeline_create(lv_anim_timeline_t * at, const lv_anim_timeline_wrapper_t * wrapper);
static void page_open_anim_timeline_create(lv_anim_timeline_t * at, const lv_anim_timeline_wrapper_t * wrapper);

#if LV_100ASK_PAGE_MANAGER_CUSTOM_ANIMARION == 0
    static void default_open_page(lv_obj_t * obj);
    static void default_close_page(lv_obj_t * obj);
#endif

static void lv_obj_100ask_open_page_anim_start_cb(lv_anim_t * a);
static void lv_obj_100ask_open_page_anim_ready_cb(lv_anim_t * a);
static lv_obj_t * get_page(lv_obj_t * page_manager, char *name);
static char * get_page_name(lv_obj_t * page_manager, lv_obj_t * page);

/**********************
 *  STATIC VARIABLES
 **********************/
lv_obj_t * g_obj_page_manager = NULL;

const lv_obj_class_t lv_100ask_page_manager_class = {
    .constructor_cb = lv_100ask_page_manager_constructor,
    .destructor_cb = lv_100ask_page_manager_destructor,
    .event_cb = lv_100ask_page_manager_event,
    .width_def = LV_PCT(100),
    .height_def = LV_PCT(100),
    .group_def = LV_OBJ_CLASS_GROUP_DEF_FALSE,
	.instance_size = sizeof(lv_100ask_page_manager_t),
    .base_class = &lv_obj_class
};


const lv_obj_class_t lv_100ask_page_manager_page_class = {
    .constructor_cb = lv_100ask_page_manager_page_constructor,
    .destructor_cb = lv_100ask_page_manager_page_destructor,
    .width_def = LV_PCT(100),
    .height_def = LV_PCT(100),
    .group_def = LV_OBJ_CLASS_GROUP_DEF_FALSE,
    .instance_size = sizeof(lv_100ask_page_manager_page_t),
    .base_class = &lv_obj_class,
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_100ask_page_manager_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

lv_obj_t * lv_100ask_page_manager_page_create(lv_obj_t * parent, char * name)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(&lv_100ask_page_manager_page_class, parent);
    lv_obj_class_init_obj(obj);

    lv_100ask_page_manager_page_t * page = (lv_100ask_page_manager_page_t *)obj;
    page->name = name;

    return obj;
}

/*=====================
 * Other functions
 *====================*/
void lv_100ask_page_manager_open_previous_page(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_100ask_page_manager_t * page_manager = (lv_100ask_page_manager_t *)obj;

    lv_ll_t * history_ll = &(page_manager->history_ll);

    /* The current page */
    lv_100ask_page_manager_history_t * act_hist = _lv_ll_get_head(history_ll);

    /* The previous page */
    lv_100ask_page_manager_history_t * prev_hist = _lv_ll_get_next(history_ll, act_hist);

    if(prev_hist != NULL) {
        lv_100ask_page_manager_set_close_page(act_hist->page, NULL);
        /* Previous page exists */
        /* Delete the current item from the history */
        _lv_ll_remove(history_ll, act_hist);
        lv_mem_free(act_hist);
        page_manager->cur_depth--;

        /* Create the previous page.
        *  Remove it from the history because `lv_100ask_page_manager_set_open_page` will add it again */
        _lv_ll_remove(history_ll, prev_hist);
        page_manager->cur_depth--;
        lv_100ask_page_manager_set_open_page(prev_hist->page, NULL);

        lv_mem_free(prev_hist);
    }
}

/*=====================
 * Setter functions
 *====================*/

void lv_100ask_page_manager_set_main_page(lv_obj_t * obj, lv_obj_t * page)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_100ask_page_manager_t * page_manager = (lv_100ask_page_manager_t *)obj;
    page_manager->main_page = page;
}

void lv_100ask_page_manager_set_page_init(lv_obj_t * obj, void (*init)(lv_obj_t  *page))
{
    lv_100ask_page_manager_page_t * page = (lv_100ask_page_manager_page_t *)obj;

    page->init = init;
}

void lv_100ask_page_manager_set_open_page(lv_obj_t * obj, char *name)
{
    lv_100ask_page_manager_t * page_manager = (lv_100ask_page_manager_t *)(g_obj_page_manager);
    lv_ll_t * history_ll = &(page_manager->history_ll);

    lv_100ask_page_manager_page_t * page;
    if (obj)
    {
        page = (lv_100ask_page_manager_page_t *)obj;
    }
    else if (name)
    {
        obj = get_page(g_obj_page_manager, name);
        page = (lv_100ask_page_manager_page_t *)obj;
    }
    else return;

    if (lv_obj_get_child_cnt(obj) == 0)
    {
        page->init(obj);
    }

    if(page->open_page)
        page->open_page(obj);

    /* del a new node */
    lv_100ask_page_manager_history_t * act_hist = _lv_ll_get_head(history_ll);
    if(act_hist != NULL)
        lv_100ask_page_manager_set_close_page(act_hist->page, NULL);

    /* Add a new node */
    lv_100ask_page_manager_history_t * new_node = _lv_ll_ins_head(history_ll);
    new_node->page = obj;
    page_manager->cur_depth++;
}

void lv_100ask_page_manager_set_close_page(lv_obj_t * obj, char *name)
{
    lv_100ask_page_manager_t * page_manager = (lv_100ask_page_manager_t *)(g_obj_page_manager);

    lv_100ask_page_manager_page_t * page;

    if (obj)
    {
        page = (lv_100ask_page_manager_page_t *)obj;
    }
    else if (name)
    {
        obj = get_page(g_obj_page_manager, name);
        page = (lv_100ask_page_manager_page_t *)obj;
    }
    else return;

    if(page->close_page)
        page->close_page(obj);

    //lv_ll_t * history_ll = &(page_manager->history_ll);
    /* The current page */
    //lv_100ask_page_manager_history_t * act_hist = _lv_ll_get_head(history_ll);

    //_lv_ll_remove(history_ll, act_hist);
    //lv_mem_free(act_hist);
    //page_manager->cur_depth--;

}

void lv_100ask_page_manager_set_load_page_event(lv_obj_t * obj, lv_obj_t * page, char *name)
{
    /* Make the object clickable */
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    /* Remove old event */
    if(lv_obj_remove_event_cb(obj, lv_page_manager_load_page_event_cb)) {
        lv_event_send(obj, LV_EVENT_DELETE, NULL);
        lv_obj_remove_event_cb(obj, lv_100ask_page_manager_page_del_event_cb);
    }

    if ((NULL == page) && (name != NULL))
    {
        page = get_page(g_obj_page_manager, name);
    }

    if (page)
    {
        lv_obj_add_event_cb(obj, lv_page_manager_load_page_event_cb, LV_EVENT_CLICKED, page);
        //lv_obj_add_event_cb(obj, lv_100ask_page_manager_page_del_event_cb, LV_EVENT_DELETE, page);
    }
}

void lv_100ask_page_manager_set_open_page_anim(lv_obj_t * obj, void (*open_anim)(lv_obj_t  * obj))
{
    lv_100ask_page_manager_page_t * page = (lv_100ask_page_manager_page_t *)obj;

    page->open_page = open_anim;
}

void lv_100ask_page_manager_set_close_page_anim(lv_obj_t * obj, void (*close_anim)(lv_obj_t  * obj))
{
    lv_100ask_page_manager_page_t * page = (lv_100ask_page_manager_page_t *)obj;

    page->close_page = close_anim;
}

/*=====================
 * Getter functions
 *====================*/

lv_obj_t * lv_100ask_page_manager_get_page(lv_obj_t * obj, char * name)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_obj_t * page = get_page(obj, name);

    return page;
}

char * lv_100ask_page_manager_get_page_name(lv_obj_t * obj, lv_obj_t * page)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    LV_ASSERT_OBJ(page, MY_PAGE_CLASS);

    char * page_name = get_page_name(obj, page);

    return page_name;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void lv_100ask_page_manager_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_100ask_page_manager_t * page_manager = (lv_100ask_page_manager_t *)obj;

    page_manager->cur_depth = 0;
    page_manager->prev_depth = 0;

    _lv_ll_init(&(page_manager->history_ll), sizeof(lv_100ask_page_manager_history_t));

    page_manager->main_page = NULL;
    g_obj_page_manager = obj;

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_100ask_page_manager_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_100ask_page_manager_t * page_manager = (lv_100ask_page_manager_t *)obj;
    lv_ll_t * history_ll = &(page_manager->history_ll);

    _lv_ll_clear(history_ll);

    g_obj_page_manager = NULL;

    LV_TRACE_OBJ_CREATE("finished");
}


static void lv_100ask_page_manager_page_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);

    lv_100ask_page_manager_page_t * page = (lv_100ask_page_manager_page_t *)obj;

    page->anim_timeline = NULL;
    page->init = NULL;
#if LV_100ASK_PAGE_MANAGER_CUSTOM_ANIMARION
    page->open_page = NULL;
    page->close_page = NULL;
#else
    page->open_page = default_open_page;
    page->close_page = default_close_page;
    page->close_page(obj);
#endif

}

static void lv_100ask_page_manager_page_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);

    lv_100ask_page_manager_page_t * page = (lv_100ask_page_manager_page_t *)obj;

    if(page->name != NULL) {
        lv_mem_free(page->name);
        page->name = NULL;
    }
}

static void lv_page_manager_load_page_event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_t * page = lv_event_get_user_data(e);

    lv_100ask_page_manager_set_open_page(page, NULL);
}

static void lv_100ask_page_manager_page_del_event_cb(lv_event_t * e)
{

}



static void lv_100ask_page_manager_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_res_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RES_OK) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
}


#if LV_100ASK_PAGE_MANAGER_CUSTOM_ANIMARION == 0
static void default_open_page(lv_obj_t * obj)
{
    lv_100ask_page_manager_page_t * page = (lv_100ask_page_manager_page_t *)obj;

    if (page->anim_timeline) {
		lv_anim_timeline_set_reverse(page->anim_timeline, 1);
        lv_anim_timeline_start(page->anim_timeline);

        lv_anim_timeline_del(page->anim_timeline);
		page->anim_timeline = NULL;
	}

    /* Create anim timeline */
	page->anim_timeline = lv_anim_timeline_create();
    //lv_obj_refr_size(obj);
    //lv_obj_refr_pos(obj);
    lv_obj_update_layout(obj);

    lv_anim_timeline_wrapper_t wrapper = ANIM_DEF(400, obj, height, 0, lv_obj_get_height(obj));
    page_open_anim_timeline_create(page->anim_timeline, &wrapper);

    lv_anim_timeline_set_reverse(page->anim_timeline, 0);
    lv_anim_timeline_start(page->anim_timeline);
}


static void default_close_page(lv_obj_t * obj)
{
    lv_100ask_page_manager_page_t * page = (lv_100ask_page_manager_page_t *)obj;

	if (page->anim_timeline) {
		lv_anim_timeline_del(page->anim_timeline);
		page->anim_timeline = NULL;
	}

	/* Create anim timeline */
	page->anim_timeline = lv_anim_timeline_create();
    //lv_obj_refr_size(obj);
    //lv_obj_refr_pos(obj);
    lv_obj_update_layout(obj);

	lv_anim_timeline_wrapper_t wrapper = ANIM_DEF(300, obj, y, -lv_obj_get_height(obj), 0);
	page_close_anim_timeline_create(page->anim_timeline, &wrapper);

	lv_anim_timeline_set_reverse(page->anim_timeline, 1);
	lv_anim_timeline_start(page->anim_timeline);
}
#endif

static void page_open_anim_timeline_create(lv_anim_timeline_t * at, const lv_anim_timeline_wrapper_t * wrapper)
{
    const lv_anim_timeline_wrapper_t * atw = wrapper;

    /* anim */
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, atw->obj);
    lv_anim_set_values(&anim, atw->start, atw->end);
    lv_anim_set_early_apply(&anim, atw->early_apply);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)atw->exec_cb);
    lv_anim_set_path_cb(&anim, atw->path_cb);
    lv_anim_set_start_cb(&anim, lv_obj_100ask_open_page_anim_start_cb);
    lv_anim_set_time(&anim, atw->duration);

    lv_anim_timeline_add(at, atw->start_time, &anim);
}

static void page_close_anim_timeline_create(lv_anim_timeline_t * at, const lv_anim_timeline_wrapper_t * wrapper)
{
    const lv_anim_timeline_wrapper_t * atw = wrapper;

    /* anim */
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, atw->obj);
    lv_anim_set_values(&anim, atw->start, atw->end);
    lv_anim_set_early_apply(&anim, atw->early_apply);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)atw->exec_cb);
    lv_anim_set_path_cb(&anim, atw->path_cb);
    lv_anim_set_time(&anim, atw->duration);

#if LV_100ASK_PAGE_MANAGER_SW_DEL_PAGE
    lv_anim_set_ready_cb(&anim, lv_obj_100ask_open_page_anim_ready_cb);
#endif

    lv_anim_timeline_add(at, atw->start_time, &anim);
}


static lv_obj_t * get_page(lv_obj_t * page_manager, char *name)
{
    uint32_t i;
    for(i = 0; i < lv_obj_get_child_cnt(page_manager); i++) {
        lv_obj_t * child = lv_obj_get_child(page_manager, i);
        lv_100ask_page_manager_page_t * page_manager_page = (lv_100ask_page_manager_page_t *)child;
        if (strcmp(name, page_manager_page->name) == 0)
        {
            return child;
            break;
        }
    }
    return NULL;
}


static char * get_page_name(lv_obj_t * page_manager, lv_obj_t * page)
{
    uint32_t i;
    for(i = 0; i < lv_obj_get_child_cnt(page_manager); i++) {
        lv_obj_t * child = lv_obj_get_child(page_manager, i);
        lv_100ask_page_manager_page_t * page_manager_page = (lv_100ask_page_manager_page_t *)child;
        if (page == child)
        {
            return page_manager_page->name;
            break;
        }
    }
    return NULL;
}

static void lv_obj_100ask_open_page_anim_start_cb(lv_anim_t * a)
{
    lv_obj_t * obj = (lv_obj_t *)a->var;
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}


static void lv_obj_100ask_open_page_anim_ready_cb(lv_anim_t * a)
{
    lv_obj_t * obj = (lv_obj_t *)a->var;
#if LV_100ASK_PAGE_MANAGER_SW_DEL_PAGE
    lv_obj_clean(obj);
#else
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
#endif
}
#endif  /*LV_USE_100ASK_PAGE_MANAGER*/
