/*
 * MIT License
 * Copyright (c) 2022 _VIFEXTech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef GBA_INTERNAL_H
#define GBA_INTERNAL_H

#include "lvgl/lvgl.h"
#include "lv_mem_macro.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GBA_ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

typedef enum {
    GBA_JOYPAD_B,
    GBA_JOYPAD_Y,
    GBA_JOYPAD_SELECT,
    GBA_JOYPAD_START,
    GBA_JOYPAD_UP,
    GBA_JOYPAD_DOWN,
    GBA_JOYPAD_LEFT,
    GBA_JOYPAD_RIGHT,
    GBA_JOYPAD_A,
    GBA_JOYPAD_X,
    GBA_JOYPAD_L,
    GBA_JOYPAD_R,
    GBA_JOYPAD_L2,
    GBA_JOYPAD_R2,
    GBA_JOYPAD_L3,
    GBA_JOYPAD_R3,
    _GBA_JOYPAD_MAX
} gba_joypad_id_t;

typedef struct gba_view_s gba_view_t;

typedef struct {
    uint32_t (*read_cb)(void* user_data);
    void* user_data;
} gba_input_event_t;

typedef struct gba_context_s {
    gba_view_t* view;
    lv_timer_t* timer;
    bool invalidate;

    struct {
        lv_coord_t fb_width;
        lv_coord_t fb_height;
        lv_coord_t fb_stride;
        double fps; /* FPS of video content. */
        double sample_rate; /* Sampling rate of audio. */
    } av_info;

    uint32_t key_state;
    lv_ll_t input_event_ll;
    size_t (*audio_output_cb)(void* user_data, const int16_t* data, size_t frames);
    void* audio_output_user_data;
} gba_context_t;

void gba_retro_init(gba_context_t* ctx);
void gba_retro_deinit(gba_context_t* ctx);
bool gba_retro_load_game(gba_context_t* ctx, const char* path);
void gba_retro_run(gba_context_t* ctx);

bool gba_view_init(gba_context_t* ctx, lv_obj_t* par, int mode);
void gba_view_deinit(gba_context_t* ctx);
lv_obj_t* gba_view_get_root(gba_context_t* ctx);
void gba_view_draw_frame(gba_context_t* ctx, const uint16_t* buf, lv_coord_t width, lv_coord_t height);
void gba_view_invalidate_frame(gba_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif
