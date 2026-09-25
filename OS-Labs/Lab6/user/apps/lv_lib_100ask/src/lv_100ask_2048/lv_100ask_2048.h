/**
 * @file lv_100ask_2048.h
 *
 */

#ifndef LV_100ASK_2048_H
#define LV_100ASK_2048_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_lib_100ask.h"

#if LV_USE_100ASK_2048 != 0

/*********************
 *      DEFINES
 *********************/
#ifdef LV_100ASK_2048_MATRIX_SIZE
    #define MATRIX_SIZE     LV_100ASK_2048_MATRIX_SIZE
#elif
    #define MATRIX_SIZE     4
#endif

/**********************
 *      TYPEDEFS
 **********************/
/*Data of canvas*/
typedef struct {
    lv_obj_t obj;
    lv_obj_t * btnm;
    uint16_t score;
    uint16_t map_count;
    uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE];
    char    * btnm_map[MATRIX_SIZE * MATRIX_SIZE + MATRIX_SIZE];
    bool     game_over;
} lv_100ask_2048_t;

/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_obj_t * lv_100ask_2048_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/
void lv_100ask_2048_set_new_game(lv_obj_t * obj);

/*=====================
 * Getter functions
 *====================*/
uint16_t lv_100ask_2048_get_best_tile(lv_obj_t * obj);

uint16_t lv_100ask_2048_get_score(lv_obj_t * obj);

bool lv_100ask_2048_get_status(lv_obj_t * obj);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#endif  /*LV_USE_100ASK_2048*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_100ASK_2048_H*/
