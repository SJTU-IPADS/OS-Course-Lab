/**
 * @file lv_100ask_memory_game.h
 *
 */

#ifndef LV_100ASK_MEMORY_GAME_H
#define LV_100ASK_MEMORY_GAME_H

#ifdef __cplusplus
extern "C" {
#endif


/*********************
 *      INCLUDES
 *********************/
#include "../../lv_lib_100ask.h"

#if LV_USE_100ASK_MEMORY_GAME != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/*Data of canvas*/
typedef struct {
    lv_obj_t obj;
    lv_obj_t * bef;
    uint16_t row;
    uint16_t col;
} lv_100ask_memory_game_t;

/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_obj_t * lv_100ask_memory_game_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/
/**
 * Generate random numbers and save to array.
 * Random number pairs that repeat only once in the specified range are generated here.
 * @param arry          An array that holds random numbers
 * @param max_count     Total number of random numbers to be generated
 * @param count         Random number range (max_count / 2)
 */
void lv_100ask_memory_game_set_map(lv_obj_t * obj, uint16_t row, uint16_t col);

/*=====================
 * Getter functions
 *====================*/

uint16_t lv_100ask_memory_game_get_row(lv_obj_t * obj);

uint16_t lv_100ask_memory_game_get_col(lv_obj_t * obj);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#endif  /*LV_100ASK_MEMORY_GAME*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_USE_100ASK_MEMORY_GAME_H*/
