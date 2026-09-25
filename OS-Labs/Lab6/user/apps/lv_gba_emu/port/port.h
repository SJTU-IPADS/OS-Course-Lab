/**
 * @file port.h
 *
 */

#ifndef PORT_H
#define PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

int lv_port_init(void);

void lv_port_sleep(uint32_t ms);

#define KEY_NUMS 150
uint8_t lv_port_keystate[KEY_NUMS];

uint32_t lv_port_tick_get(void);

void gba_port_init(lv_obj_t* gba_emu);

int gba_audio_init(lv_obj_t* gba_emu);

void gba_audo_deinit(lv_obj_t* gba_emu);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*PORT_H*/
