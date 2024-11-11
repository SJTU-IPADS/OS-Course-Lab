/**
 * @file simple_test.h
 *
 */

#ifndef LV_100ASK_NES_SIMPLE_TEST_H
#define LV_100ASK_NES_SIMPLE_TEST_H

#ifdef __cplusplus
extern "C" {
#endif


/*********************
 *      INCLUDES
 *********************/
#include "../../lv_lib_100ask.h"

#if LV_100ASK_NES_SIMPLE_TEST != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
enum {
    LV_MENU_ITEM_BUILDER_VARIANT_1,
    LV_MENU_ITEM_BUILDER_VARIANT_2
};
typedef uint8_t lv_menu_builder_variant_t;

/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_100ask_nes_simple_test(void);

/*=====================
 * Setter functions
 *====================*/

/*=====================
 * Getter functions
 *====================*/

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#endif  /*LV_100ASK_NES_SIMPLE_TEST*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_100ASK_NES_SIMPLE_TEST_H*/
