/**
 * @file save_as_bmp.h
 *
 */

#ifndef SAVE_AS_BMP_H
#define SAVE_AS_BMP_H

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

/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
bool save_as_bmp_file(uint8_t * image, uint32_t w, uint32_t h, uint32_t bpp, char *filename);

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

#endif /*SAVE_AS_BMP_H*/
