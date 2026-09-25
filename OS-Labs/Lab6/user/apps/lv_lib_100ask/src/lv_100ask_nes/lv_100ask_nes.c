/**
 * @file lv_100ask_nes.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_100ask_nes.h"

#if LV_USE_100ASK_NES

#if LV_100ASK_NES_PLATFORM_POSIX
	#include <unistd.h>
#elif LV_100ASK_NES_PLATFORM_FREERTOS
#endif

#include <stdio.h>
#include <stdlib.h>

#include "InfoNES.h"
#include "InfoNES_System.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_100ask_nes_class

/* Pad state */
extern DWORD dwPad1;
extern DWORD dwPad2;
extern DWORD dwSystem;

/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_100ask_nes_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_100ask_nes_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

static void lv_100ask_nes_event(const lv_obj_class_t * class_p, lv_event_t * e);
//static void lv_100ask_nes_draw_event(lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_100ask_nes_class = {
    .constructor_cb = lv_100ask_nes_constructor,
    .destructor_cb = lv_100ask_nes_destructor,
    .event_cb = lv_100ask_nes_event,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_100ask_nes_t),
    .base_class = &lv_obj_class
};


// Used by infones
static lv_obj_t * nes_obj = NULL;

#if LV_COLOR_DEPTH == 16
static uint16_t nes_dbuf[256*240];
#else LV_COLOR_DEPTH == 32
// draw buffer
static uint16_t nes_dbuf[2*256*240];
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_100ask_nes_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}


/*=====================
 * Setter functions
 *====================*/
void lv_100ask_nes_set_fn(lv_obj_t * obj, char * fn)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)obj;

    uint16_t fn_len = strlen(fn);

    if((!fn) || (fn_len == 0)) return;

    if(nes->cur_fn) {
        if(strlen(nes->cur_fn) < fn_len) {
            lv_mem_free(nes->cur_fn);
            nes->cur_fn = NULL;
        }
        else {
            lv_memset_00(nes->cur_fn, strlen(nes->cur_fn));
        }
    }

    nes->cur_fn = lv_mem_alloc(fn_len + 1);
    LV_ASSERT_MALLOC(nes->cur_fn);
    if(nes->cur_fn == NULL) return;

    strcpy(nes->cur_fn, fn);
}


void lv_100ask_nes_set_state(lv_obj_t * obj, lv_100ask_nes_state_t state)
{
    //LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)obj;

    nes->state = state;
}

void lv_100ask_nes_set_zoom(lv_obj_t * obj, uint16_t zoom)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)obj;

    lv_img_set_zoom(nes->img, zoom);
}

void lv_100ask_nes_set_speed(lv_obj_t * obj, uint32_t speed)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)obj;

    nes->speed = speed;
}

void lv_100ask_nes_set_lock(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)obj;

#if LV_100ASK_NES_PLATFORM_POSIX
    pthread_mutex_lock(&(nes->mutex));
#elif LV_100ASK_NES_PLATFORM_FREERTOS
    BaseType_t xStatus;
    while(xStatus != pdTRUE) {
        xStatus = xSemaphoreTakeRecursive(nes->mutex, portMAX_DELAY);
        printf("Take the Mutex in lv_100ask_nes_set_lock %s\r\n", (xStatus == pdTRUE)? "Success" : "Failed");
        vTaskDelay(xTicksToWait);
    }
#endif

}


void lv_100ask_nes_set_unlock(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)obj;

#if LV_100ASK_NES_PLATFORM_POSIX
    pthread_mutex_unlock(&(nes->mutex));
#elif LV_100ASK_NES_PLATFORM_FREERTOS
    xSemaphoreGiveRecursive(nes->mutex);
#endif

}

void lv_100ask_nes_set_key(lv_obj_t * obj, lv_100ask_nes_key_t key, lv_100ask_nes_key_state_t state)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)obj;

    if (state == LV_100ASK_NES_KEY_STATE_PRESSED) {
        switch (key)
        {
            case LV_100ASK_NES_KEY_A:
                dwPad1 |= (1<<0);
                break;
            case LV_100ASK_NES_KEY_B:
                dwPad1 |= (1<<1);
                break;
            case LV_100ASK_NES_KEY_START:
                dwPad1 |= (1<<3);
                break;
            case LV_100ASK_NES_KEY_SELECT:
                dwPad1 |= (1<<2);
                break;
            case LV_100ASK_NES_KEY_UP:
                dwPad1 |= (1<<4);
                break;
            case LV_100ASK_NES_KEY_DOWN:
                dwPad1 |= (1<<5);
                break;
            case LV_100ASK_NES_KEY_LEFT:
                dwPad1 |= (1<<6);
                break;
            case LV_100ASK_NES_KEY_RIGHT:
                dwPad1 |= (1<<7);
                break;
            default:
                break;
        }
    }
    if (state == LV_100ASK_NES_KEY_STATE_RELEASED) {
        switch (key)
        {
            case LV_100ASK_NES_KEY_A:
                dwPad1 &= ~(1<<0);
                break;
            case LV_100ASK_NES_KEY_B:
                dwPad1 &= ~(1<<1);
                break;
            case LV_100ASK_NES_KEY_START:
                dwPad1 &= ~(1<<3);
                break;
            case LV_100ASK_NES_KEY_SELECT:
                dwPad1 &= ~(1<<2);
                break;
            case LV_100ASK_NES_KEY_UP:
                dwPad1 &= ~(1<<4);
                break;
            case LV_100ASK_NES_KEY_DOWN:
                dwPad1 &= ~(1<<5);
                break;
            case LV_100ASK_NES_KEY_LEFT:
                dwPad1 &= ~(1<<6);
                break;
            case LV_100ASK_NES_KEY_RIGHT:
                dwPad1 &= ~(1<<7);
                break;
            case LV_100ASK_NES_KEY_MENU:
                dwSystem |= PAD_SYS_QUIT;
                if(nes->state == LV_100ASK_NES_STATE_NORMAL) {
                    lv_100ask_nes_set_lock(obj);
                    nes->state = LV_100ASK_NES_STATE_MENU;
                }

                break;
            default:
                break;
        }
    }

}

/*=====================
 * Getter functions
 *====================*/
char * lv_100ask_nes_get_fn(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)obj;

    return nes->cur_fn;
}

uint32_t lv_100ask_nes_get_speed(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)obj;

    return nes->speed;
}

lv_100ask_nes_mutex_t lv_100ask_nes_get_mutex(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)obj;

    return nes->mutex;
}

lv_100ask_nes_state_t lv_100ask_nes_get_state(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)obj;

    return nes->state;
}
/*=====================
 * Other functions
 *====================*/

void lv_100ask_nes_menu(void)
{
    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)nes_obj;

    if(nes->state == LV_100ASK_NES_STATE_MENU) {
        lv_100ask_nes_set_lock(nes_obj);
        if(nes->state == LV_100ASK_NES_STATE_NEW_GAME) {
            if ( InfoNES_ReadRom( nes->cur_fn ) == 0 )
                InfoNES_Reset();
        }
        dwSystem |= 0;
        nes->state = LV_100ASK_NES_STATE_NORMAL;
        lv_100ask_nes_set_unlock(nes_obj);
    }
}

void lv_100ask_nes_run(void * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    nes_obj = (lv_obj_t *)obj;

    lv_100ask_nes_set_lock(nes_obj);
    // Wait for the user to select the file when starting for the first time
    lv_100ask_nes_set_unlock(nes_obj);
    lv_100ask_nes_set_state(nes_obj, LV_100ASK_NES_STATE_NORMAL);

    if (start_application(lv_100ask_nes_get_fn(nes_obj)))
    {
      /* MainLoop */
      InfoNES_Main();

      /* End */
      SaveSRAM();
    }
    else
    {
      /* Not a NES format file */
      LV_LOG_ERROR("%s isn't a NES format file!", lv_100ask_nes_get_fn(nes_obj));
    }
}


void lv_100ask_nes_flush(void)
{
    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)nes_obj;
    uint32_t x;
    uint32_t y;
    uint32_t index = 0;

#if LV_COLOR_DEPTH == 16
    //uint16_t *fb = (unsigned short *)&WorkFrame;
    for(y =0; y < 240; y++) {
        for(x = 0; x < 256; x++) {
            //fb[index] = (WorkFrame[index] >> 3) << 4 | (WorkFrame[index]&0x001f);
            nes_dbuf[index] = (WorkFrame[index] >> 3) << 4 | (WorkFrame[index]&0x001f);
            index++;
        }
    }
#else if LV_COLOR_DEPTH == 32
    uint8_t red, green, blue, cov;
    uint32_t nes_buf_index = 0;

    for(y =0; y < 240; y++) {
        for(x = 0; x < 256; x++) {
            red   = (WorkFrame[index] & 0xff0000) >> 16;
            green = (WorkFrame[index] & 0xff00) >> 8;
            blue  = (WorkFrame[index] & 0xff >> 0);
            //WorkFrame[index] = (green<<24) | (blue<<16) | (red << 8);

            nes_dbuf[nes_buf_index++] = ((green << 8) | blue);      // l: G B
            nes_dbuf[nes_buf_index++] = ((0xff << 8) | red);      // h: A R
            index++;
        }
    }
#endif
    if(nes->state == LV_100ASK_NES_STATE_NORMAL) {
        lv_100ask_nes_set_lock(nes_obj);
        nes->dsc->data = (const uint8_t *)nes_dbuf;
        //obj->dsc->data = (const uint8_t *)WorkFrame;
        lv_img_set_src(nes->img, nes->dsc);
        lv_100ask_nes_set_unlock(nes_obj);
#if LV_100ASK_NES_PLATFORM_POSIX
        usleep(lv_100ask_nes_get_speed(nes_obj));
#elif LV_100ASK_NES_PLATFORM_FREERTOS
        vTaskDelay(lv_100ask_nes_get_speed(nes_obj));
#endif
    }

}


/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_100ask_nes_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)obj;

    nes->state = LV_100ASK_NES_STATE_NORMAL;
    nes->speed = 0;

    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));

    nes->dsc = lv_mem_alloc(sizeof(lv_img_dsc_t));
    lv_memset_00(nes->dsc, sizeof(lv_img_dsc_t));
    nes->dsc->data = NULL;
#if LV_COLOR_DEPTH == 16
    nes->dsc->data_size = 2*256*240;
#else if LV_COLOR_DEPTH == 32
    nes->dsc->data_size = 4*256*240;
#endif
    nes->dsc->header.w = 256;
    nes->dsc->header.h = 240;
    nes->dsc->header.cf = LV_IMG_CF_TRUE_COLOR;

    nes->img = lv_img_create(lv_scr_act());
    lv_img_set_antialias(nes->img, true);
    lv_obj_center(nes->img);
    lv_img_set_src(nes->img, nes->dsc);
    //lv_img_set_zoom(nes->img, 512);
    //lv_obj_add_event_cb(nes->img, lv_100ask_nes_draw_event, LV_EVENT_ALL, obj);

#if LV_100ASK_NES_PLATFORM_POSIX
    pthread_mutex_init(&(nes->mutex), NULL);
#elif LV_100ASK_NES_PLATFORM_FREERTOS
    nes->mutex = xSemaphoreCreateRecursiveMutex();
#endif

    LV_TRACE_OBJ_CREATE("finished");
}


static void lv_100ask_nes_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)obj;

}


static void lv_100ask_nes_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_res_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RES_OK) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    lv_100ask_nes_t * nes = (lv_100ask_nes_t *)obj;
}

#if 0
static void lv_100ask_nes_draw_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_user_data(e);
    //lv_obj_t * img = lv_event_get_target(e);

    if(code == LV_EVENT_DRAW_MAIN_BEGIN) {
        lv_100ask_nes_set_lock(obj);
    }
    else if(code == LV_EVENT_DRAW_MAIN_END) {
        lv_100ask_nes_set_unlock(obj);
    }
}
#endif

#endif  /*LV_USE_100ASK_NES*/
