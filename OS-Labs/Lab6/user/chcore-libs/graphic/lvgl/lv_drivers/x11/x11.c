/**
 * @file x11.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "x11.h"
#if USE_X11
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/*********************
 *      DEFINES
 *********************/
#ifndef KEYBOARD_BUFFER_SIZE
#define KEYBOARD_BUFFER_SIZE 64
#endif

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

#ifndef X11_OPTIMIZED_SCREEN_UPDATE
#define X11_OPTIMIZED_SCREEN_UPDATE 1
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static Display *display = NULL;
static Window window = (XID)-1;
static GC gc = NULL;
static XImage *ximage = NULL;
static lv_timer_t *timer = NULL;

static char kb_buffer[KEYBOARD_BUFFER_SIZE];
static lv_point_t mouse_pos = {0, 0};
static bool left_mouse_btn = false;
static bool right_mouse_btn = false;
static bool wheel_mouse_btn = false;
static int16_t wheel_cnt = 0;

/**********************
 *      MACROS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/
static int predicate(Display *disp, XEvent *evt, XPointer arg)
{
        return 1;
}

static void x11_event_handler(lv_timer_t *t)
{
        XEvent myevent;
        KeySym mykey;
        int n;

        /* handle all outstanding X events */
        while (XCheckIfEvent(display, &myevent, predicate, NULL)) {
                switch (myevent.type) {
                case Expose:
                        if (myevent.xexpose.count == 0) {
                                XPutImage(display,
                                          window,
                                          gc,
                                          ximage,
                                          0,
                                          0,
                                          0,
                                          0,
                                          LV_HOR_RES,
                                          LV_VER_RES);
                        }
                        break;
                case MotionNotify:
                        mouse_pos.x = myevent.xmotion.x;
                        mouse_pos.y = myevent.xmotion.y;
                        break;
                case ButtonPress:
                        switch (myevent.xbutton.button) {
                        case Button1:
                                left_mouse_btn = true;
                                break;
                        case Button2:
                                wheel_mouse_btn = true;
                                break;
                        case Button3:
                                right_mouse_btn = true;
                                break;
                        case Button4:
                                wheel_cnt--; // Scrolled up
                                break;
                        case Button5:
                                wheel_cnt++; // Scrolled down
                                break;
                        default:
                                LV_LOG_WARN("unhandled button press : %d",
                                            myevent.xbutton.button);
                        }
                        break;
                case ButtonRelease:
                        switch (myevent.xbutton.button) {
                        case Button1:
                                left_mouse_btn = false;
                                break;
                        case Button2:
                                wheel_mouse_btn = false;
                                break;
                        case Button3:
                                right_mouse_btn = false;
                                break;
                        }
                        break;
                case KeyPress:
                        n = XLookupString(&myevent.xkey,
                                          &kb_buffer[0],
                                          sizeof(kb_buffer),
                                          &mykey,
                                          NULL);
                        kb_buffer[n] = '\0';
                        break;
                case KeyRelease:
                        break;
                default:
                        LV_LOG_WARN("unhandled x11 event: %d", myevent.type);
                }
        }
}

static void lv_x11_hide_cursor()
{
        XColor black = {.red = 0, .green = 0, .blue = 0};
        char empty_data[] = {0};

        Pixmap empty_bitmap =
                XCreateBitmapFromData(display, window, empty_data, 1, 1);
        Cursor inv_cursor = XCreatePixmapCursor(
                display, empty_bitmap, empty_bitmap, &black, &black, 0, 0);
        XDefineCursor(display, window, inv_cursor);
        XFreeCursor(display, inv_cursor);
        XFreePixmap(display, empty_bitmap);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_x11_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area,
                  lv_color_t *color_p)
{
#if X11_OPTIMIZED_SCREEN_UPDATE
        static const lv_area_t inv_area = {
                .x1 = 0xFFFF, .x2 = 0, .y1 = 0xFFFF, .y2 = 0};
        static lv_area_t upd_area = inv_area;

        /* build display update area until lv_disp_flush_is_last */
        upd_area.x1 = MIN(upd_area.x1, area->x1);
        upd_area.x2 = MAX(upd_area.x2, area->x2);
        upd_area.y1 = MIN(upd_area.y1, area->y1);
        upd_area.y2 = MAX(upd_area.y2, area->y2);
#endif // X11_OPTIMIZED_SCREEN_UPDATE

        for (lv_coord_t y = area->y1; y <= area->y2; y++) {
                uint32_t dst_offs = area->x1 + y * LV_HOR_RES;
                uint32_t *dst_data = &((uint32_t *)ximage->data)[dst_offs];
                for (lv_coord_t x = area->x1; x <= area->x2;
                     x++, color_p++, dst_data++) {
                        *dst_data = lv_color_to32(*color_p);
                }
        }

        if (lv_disp_flush_is_last(disp_drv)) {
#if X11_OPTIMIZED_SCREEN_UPDATE
                /* refresh collected display update area only */
                lv_coord_t upd_w = upd_area.x2 - upd_area.x1 + 1;
                lv_coord_t upd_h = upd_area.y2 - upd_area.y1 + 1;
                XPutImage(display,
                          window,
                          gc,
                          ximage,
                          upd_area.x1,
                          upd_area.y1,
                          upd_area.x1,
                          upd_area.y1,
                          upd_w,
                          upd_h);
                /* invalidate collected area */
                upd_area = inv_area;
#else
                /* refresh full display */
                XPutImage(display,
                          window,
                          gc,
                          ximage,
                          0,
                          0,
                          0,
                          0,
                          LV_HOR_RES,
                          LV_VER_RES);
#endif
        }
        lv_disp_flush_ready(disp_drv);
}

void lv_x11_get_pointer(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
        (void)indev_drv; // Unused

        data->point = mouse_pos;
        data->state = left_mouse_btn ? LV_INDEV_STATE_PRESSED :
                                       LV_INDEV_STATE_RELEASED;
}

void lv_x11_get_mousewheel(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
        (void)indev_drv; // Unused

        data->state = wheel_mouse_btn ? LV_INDEV_STATE_PRESSED :
                                        LV_INDEV_STATE_RELEASED;
        data->enc_diff = wheel_cnt;
        wheel_cnt = 0;
}

void lv_x11_get_keyboard(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
        (void)indev_drv; // Unused

        size_t len = strlen(kb_buffer);
        if (len > 0) {
                data->state = LV_INDEV_STATE_PRESSED;
                data->key = kb_buffer[0];
                memmove(kb_buffer, kb_buffer + 1, len);
                data->continue_reading = (len > 0);
        } else {
                data->state = LV_INDEV_STATE_RELEASED;
        }
}

void lv_x11_init(char const *title, lv_coord_t width, lv_coord_t height)
{
        /* setup display/screen */
        display = XOpenDisplay(NULL);
        int screen = DefaultScreen(display);

        /* drawing contexts for an window */
        unsigned long myforeground = BlackPixel(display, screen);
        unsigned long mybackground = WhitePixel(display, screen);

        /* create window */
        window = XCreateSimpleWindow(display,
                                     DefaultRootWindow(display),
                                     0,
                                     0,
                                     width,
                                     height,
                                     0,
                                     myforeground,
                                     mybackground);

        /* window manager properties (yes, use of StdProp is obsolete) */
        XSetStandardProperties(
                display, window, title, NULL, None, NULL, 0, NULL);

        /* allow receiving mouse and keyboard events */
        XSelectInput(display,
                     window,
                     PointerMotionMask | ButtonPressMask | ButtonReleaseMask
                             | KeyPressMask | KeyReleaseMask | ExposureMask);

        /* graphics context */
        gc = XCreateGC(display, window, 0, 0);

        lv_x11_hide_cursor();

        /* create cache XImage */
        Visual *visual = XDefaultVisual(display, screen);
        int dplanes = DisplayPlanes(display, screen);
        ximage = XCreateImage(display,
                              visual,
                              dplanes,
                              ZPixmap,
                              0,
                              malloc(width * height * sizeof(uint32_t)),
                              width,
                              height,
                              32,
                              0);

        timer = lv_timer_create(x11_event_handler, 10, NULL);

        /* finally bring window on top of the other windows */
        XMapRaised(display, window);
}

void lv_x11_deinit(void)
{
        lv_timer_del(timer);

        free(ximage->data);

        XDestroyImage(ximage);
        ximage = NULL;

        XFreeGC(display, gc);
        gc = NULL;
        XDestroyWindow(display, window);
        window = (XID)-1;

        XCloseDisplay(display);
        display = NULL;
}

#endif // USE_X11
