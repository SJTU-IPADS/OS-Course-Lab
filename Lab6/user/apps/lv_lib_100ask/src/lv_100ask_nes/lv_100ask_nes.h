/**
 * @file lv_100ask_nes.h
 *
 */

#ifndef LV_100ASK_NES_H
#define LV_100ASK_NES_H

#ifdef __cplusplus
extern "C" {
#endif


/*********************
 *      INCLUDES
 *********************/
#include "../../lv_lib_100ask.h"

#if LV_USE_100ASK_NES != 0

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
#if LV_100ASK_NES_PLATFORM_POSIX
    typedef pthread_mutex_t lv_100ask_nes_mutex_t;
#elif LV_100ASK_NES_PLATFORM_FREERTOS
    typedef SemaphoreHandle_t lv_100ask_nes_mutex_t
#endif

typedef enum {
    LV_100ASK_NES_KEY_A,
    LV_100ASK_NES_KEY_B,
    LV_100ASK_NES_KEY_START,
    LV_100ASK_NES_KEY_SELECT,
    LV_100ASK_NES_KEY_UP,
    LV_100ASK_NES_KEY_DOWN,
    LV_100ASK_NES_KEY_LEFT,
    LV_100ASK_NES_KEY_RIGHT,
    LV_100ASK_NES_KEY_MENU,
} lv_100ask_nes_key_t;

typedef enum {
    LV_100ASK_NES_KEY_STATE_PRESSED,
    LV_100ASK_NES_KEY_STATE_RELEASED,
} lv_100ask_nes_key_state_t;

typedef enum {
    LV_100ASK_NES_STATE_NORMAL,
    LV_100ASK_NES_STATE_RESET,
    LV_100ASK_NES_STATE_NEW_GAME,
    LV_100ASK_NES_STATE_PAUSE,
    LV_100ASK_NES_STATE_SAVE,
    LV_100ASK_NES_STATE_MENU,
} lv_100ask_nes_state_t;

/*Data of canvas*/
typedef struct {
    lv_obj_t obj;
    lv_obj_t * canvas;
    lv_draw_img_dsc_t img_rect_dsc;
    lv_img_dsc_t * dsc;
    lv_obj_t * img;
    void * nes_pixels;
    char * cur_fn;
    uint32_t speed;
    lv_100ask_nes_mutex_t mutex;
    lv_100ask_nes_state_t state;
} lv_100ask_nes_t;

extern const lv_obj_class_t lv_100ask_nes_class;

/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_obj_t * lv_100ask_nes_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/
void lv_100ask_nes_set_state(lv_obj_t * obj, lv_100ask_nes_state_t state);

void lv_100ask_nes_set_zoom(lv_obj_t * obj, uint16_t zoom);

void lv_100ask_nes_set_speed(lv_obj_t * obj, uint32_t speed);

void lv_100ask_nes_set_fn(lv_obj_t * obj, char * fn);

void lv_100ask_nes_set_key(lv_obj_t * obj, lv_100ask_nes_key_t key, lv_100ask_nes_key_state_t state);

void lv_100ask_nes_set_lock(lv_obj_t * obj);

void lv_100ask_nes_set_unlock(lv_obj_t * obj);

/*=====================
 * Getter functions
 *====================*/

uint32_t lv_100ask_nes_get_speed(lv_obj_t * obj);

char * lv_100ask_nes_get_fn(lv_obj_t * obj);

lv_100ask_nes_state_t lv_100ask_nes_get_state(lv_obj_t * obj);

lv_100ask_nes_mutex_t lv_100ask_nes_get_mutex(lv_obj_t * obj);

/*=====================
 * Other functions
 *====================*/

void lv_100ask_nes_menu(void);

void lv_100ask_nes_run(void * obj);

void lv_100ask_nes_flush(void);

/**********************
 *      MACROS
 **********************/


#endif  /*LV_USE_100ASK_NES*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_100ASK_NES_H*/
