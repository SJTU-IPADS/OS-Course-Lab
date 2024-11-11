/**
 * @file evdev.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "evdev.h"
#if USE_EVDEV != 0 || USE_BSD_EVDEV

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#if USE_BSD_EVDEV
#include <dev/evdev/input.h>
#else
#include <linux/input.h>
#endif

#if USE_XKB
#include "xkb.h"
#endif /* USE_XKB */

/**********************
 *  STATIC VARIABLES
 **********************/

evdev_device_t global_dsc;

/**********************
 *  STATIC FUNCTIONS
 **********************/

static int _evdev_process_key(uint16_t code, bool pressed)
{
#if USE_XKB
        return xkb_process_key(code, pressed);
#else
        LV_UNUSED(pressed);
        switch (code) {
        case KEY_UP:
                return LV_KEY_UP;
        case KEY_DOWN:
                return LV_KEY_DOWN;
        case KEY_RIGHT:
                return LV_KEY_RIGHT;
        case KEY_LEFT:
                return LV_KEY_LEFT;
        case KEY_ESC:
                return LV_KEY_ESC;
        case KEY_DELETE:
                return LV_KEY_DEL;
        case KEY_BACKSPACE:
                return LV_KEY_BACKSPACE;
        case KEY_ENTER:
                return LV_KEY_ENTER;
        case KEY_NEXT:
        case KEY_TAB:
                return LV_KEY_NEXT;
        case KEY_PREVIOUS:
                return LV_KEY_PREV;
        case KEY_HOME:
                return LV_KEY_HOME;
        case KEY_END:
                return LV_KEY_END;
        default:
                return 0;
        }
#endif /*USE_XKB*/
}

static int _evdev_calibrate(int v, int in_min, int in_max, int out_min,
                            int out_max)
{
        if (in_min != in_max)
                v = (v - in_min) * (out_max - out_min) / (in_max - in_min)
                    + out_min;
        return LV_CLAMP(out_min, v, out_max);
}

static lv_point_t _evdev_process_pointer(lv_indev_drv_t* drv, int x, int y)
{
        evdev_device_t* dsc = drv->user_data ? drv->user_data : &global_dsc;

        int swapped_x = dsc->swap_axes ? y : x;
        int swapped_y = dsc->swap_axes ? x : y;

        int offset_x = 0; /*Not lv_disp_get_offset_x(drv->disp) for bc*/
        int offset_y = 0; /*Not lv_disp_get_offset_y(drv->disp) for bc*/
        int width = lv_disp_get_hor_res(drv->disp);
        int height = lv_disp_get_ver_res(drv->disp);

        lv_point_t p;
        p.x = _evdev_calibrate(swapped_x,
                               dsc->hor_min,
                               dsc->hor_max,
                               offset_x,
                               offset_x + width - 1);
        p.y = _evdev_calibrate(swapped_y,
                               dsc->ver_min,
                               dsc->ver_max,
                               offset_y,
                               offset_y + height - 1);
        return p;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void evdev_init()
{
        evdev_device_init(&global_dsc);
#ifdef EVDEV_SWAP_AXES
        evdev_device_set_swap_axes(&global_dsc, EVDEV_SWAP_AXES);
#endif
#if EVDEV_CALIBRATE
        evdev_device_set_calibration(&global_dsc,
                                     EVDEV_HOR_MIN,
                                     EVDEV_VER_MIN,
                                     EVDEV_HOR_MAX,
                                     EVDEV_VER_MAX);
#endif
        evdev_device_set_file(&global_dsc, EVDEV_NAME);
}

void evdev_device_init(evdev_device_t* dsc)
{
        lv_memset(dsc, 0, sizeof(evdev_device_t));
        dsc->fd = -1;

#if USE_XKB
        xkb_init();
#endif
}

bool evdev_set_file(const char* dev_path)
{
        return evdev_device_set_file(&global_dsc, dev_path);
}

bool evdev_device_set_file(evdev_device_t* dsc, const char* dev_path)
{
        /*Reset state*/
        dsc->root_x = 0;
        dsc->root_y = 0;
        dsc->key = 0;
        dsc->state = LV_INDEV_STATE_RELEASED;

        /*Close previous*/
        if (dsc->fd >= 0) {
                close(dsc->fd);
                dsc->fd = -1;
        }
        if (!dev_path)
                return false;

        /*Open new*/
        dsc->fd = open(dev_path, O_RDONLY | O_NOCTTY | O_CLOEXEC);
        if (dsc->fd < 0) {
                LV_LOG_ERROR("open failed: %s", strerror(errno));
                return false;
        }
        if (fcntl(dsc->fd, F_SETFL, O_NONBLOCK) < 0) {
                LV_LOG_ERROR("fcntl failed: %s", strerror(errno));
                close(dsc->fd);
                dsc->fd = -1;
                return false;
        }

        return true;
}

void evdev_device_set_swap_axes(evdev_device_t* dsc, bool swap_axes)
{
        dsc->swap_axes = swap_axes;
}

void evdev_device_set_calibration(evdev_device_t* dsc, int ver_min, int hor_min,
                                  int ver_max, int hor_max)
{
        dsc->ver_min = ver_min;
        dsc->hor_min = hor_min;
        dsc->ver_max = ver_max;
        dsc->hor_max = hor_max;
}

void evdev_read(lv_indev_drv_t* drv, lv_indev_data_t* data)
{
        evdev_device_t* dsc = drv->user_data ? drv->user_data : &global_dsc;
        if (dsc->fd < 0)
                return;

        /*Update dsc with buffered events*/
        struct input_event in = {0};
        while (read(dsc->fd, &in, sizeof(in)) > 0) {
                if (in.type == EV_REL) {
                        if (in.code == REL_X)
                                dsc->root_x += in.value;
                        else if (in.code == REL_Y)
                                dsc->root_y += in.value;
                } else if (in.type == EV_ABS) {
                        if (in.code == ABS_X || in.code == ABS_MT_POSITION_X)
                                dsc->root_x = in.value;
                        else if (in.code == ABS_Y
                                 || in.code == ABS_MT_POSITION_Y)
                                dsc->root_y = in.value;
                        else if (in.code == ABS_MT_TRACKING_ID) {
                                if (in.value == -1)
                                        dsc->state = LV_INDEV_STATE_RELEASED;
                                else if (in.value == 0)
                                        dsc->state = LV_INDEV_STATE_PRESSED;
                        }
                } else if (in.type == EV_KEY) {
                        if (in.code == BTN_MOUSE || in.code == BTN_TOUCH) {
                                if (in.value == 0)
                                        dsc->state = LV_INDEV_STATE_RELEASED;
                                else if (in.value == 1)
                                        dsc->state = LV_INDEV_STATE_PRESSED;
                        } else {
                                dsc->key = _evdev_process_key(in.code,
                                                              in.value != 0);
                                if (dsc->key) {
                                        dsc->state =
                                                in.value ?
                                                        LV_INDEV_STATE_PRESSED :
                                                        LV_INDEV_STATE_RELEASED;
                                        data->continue_reading =
                                                true; /*Keep following events in
                                                         buffer for now*/
                                        break;
                                }
                        }
                }
        }

        /*Process and store in data*/
        switch (drv->type) {
        case LV_INDEV_TYPE_KEYPAD:
                data->state = dsc->state;
                data->key = dsc->key;
                break;
        case LV_INDEV_TYPE_POINTER:
                data->state = dsc->state;
                data->point =
                        _evdev_process_pointer(drv, dsc->root_x, dsc->root_y);
                break;
        default:
                break;
        }
}

#endif /*USE_EVDEV*/
