/**
 * @file x11.h
 *
 */

#ifndef X11_H
#define X11_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifndef LV_DRV_NO_CONF
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lv_drv_conf.h"
#else
#include "../../lv_drv_conf.h"
#endif
#endif

#if USE_X11

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_x11_init(char const *title, lv_coord_t width, lv_coord_t height);
void lv_x11_deinit(void);
void lv_x11_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area,
                  lv_color_t *color_p);
void lv_x11_get_pointer(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
void lv_x11_get_mousewheel(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
void lv_x11_get_keyboard(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

/**********************
 *      MACROS
 **********************/

#endif /* USE_X11 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* X11_H */
