/**
 * @file evdev.h
 *
 */

#ifndef EVDEV_H
#define EVDEV_H

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

#if USE_EVDEV || USE_BSD_EVDEV

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
        /*Device*/
        int fd;
        /*Config*/
        bool swap_axes;
        int ver_min;
        int hor_min;
        int ver_max;
        int hor_max;
        /*State*/
        int root_x;
        int root_y;
        int key;
        lv_indev_state_t state;
} evdev_device_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the global evdev device, as configured with EVDEV_NAME,
 * EVDEV_SWAP_AXES, and EVDEV_CALIBRATE.
 */
void evdev_init();
/**
 * Initialize an evdev device.
 * @param dsc evdev device
 */
void evdev_device_init(evdev_device_t* dsc);

/**
 * Reconfigure the path for the global evdev device.
 * @param dev_path device path, e.g., /dev/input/event0, or NULL to close
 * @return whether the device was successfully opened
 */
bool evdev_set_file(const char* dev_path);
/**
 * Configure or reconfigure the path for an evdev device.
 * @param dsc evdev device
 * @param dev_path device path, e.g., /dev/input/event0, or NULL to close
 * @return whether the device was successfully opened
 */
bool evdev_device_set_file(evdev_device_t* dsc, const char* dev_path);

/**
 * Configure whether pointer coordinates of an evdev device sould be swapped.
 * Default to false.
 * @param dsc evdev device
 * @param swap_axes whether to swap x and y axes
 */
void evdev_device_set_swap_axes(evdev_device_t* dsc, bool swap_axes);

/**
 * Configure a coordinate transformation for an evdev device. Applied after
 * axis swap, if any. Defaults to no transformation.
 * @param dsc evdev device
 * @param hor_min horizontal pointer coordinate mapped to 0
 * @param ver_min vertical pointer coordinate mapped to 0
 * @param ver_min pointer coordinate mapped to horizontal max of display
 * @param ver_max pointer coordinate mapped to vertical max of display
 */
void evdev_device_set_calibration(evdev_device_t* dsc, int hor_min, int ver_min,
                                  int hor_max, int ver_max);

/**
 * Read callback for the input driver.
 * @param drv input driver where drv->user_data is NULL for the global evdev
 *            device or an evdev_device_t pointer.
 * @param data destination for input events
 */
void evdev_read(lv_indev_drv_t* drv, lv_indev_data_t* data);

#endif /* USE_EVDEV */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* EVDEV_H */
