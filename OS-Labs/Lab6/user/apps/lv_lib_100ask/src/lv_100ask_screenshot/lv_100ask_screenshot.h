/**
 * @file lv_100ask_screenshot.h
 *
 */

#ifndef LV_100ASK_SCREENSHOT_H
#define LV_100ASK_SCREENSHOT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../lv_lib_100ask.h"

#if LV_USE_100ASK_SCREENSHOT != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
	LV_100ASK_SCREENSHOT_SV_BMP  = 0,
	LV_100ASK_SCREENSHOT_SV_PNG  = 1,
	LV_100ASK_SCREENSHOT_SV_JPEG = 2,
	LV_100ASK_SCREENSHOT_SV_LAST
}lv_100ask_screenshot_sv_t;

/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
bool lv_100ask_screenshot_create(lv_obj_t * obj,  lv_img_cf_t cf, lv_100ask_screenshot_sv_t screenshot_sv, const char * filename);

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

#endif  /*LV_USE_SCREENSHOT*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_SCREENSHOT_H*/
