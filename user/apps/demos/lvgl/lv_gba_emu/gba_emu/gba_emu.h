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
#ifndef LV_GBA_EMU_H
#define LV_GBA_EMU_H

#include "lvgl/lvgl.h"
#include "lv_mem_macro.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    LV_GBA_VIEW_MODE_SIMPLE,
    LV_GBA_VIEW_MODE_VIRTUAL_KEYPAD,
} lv_gba_view_mode_t;

typedef uint32_t (*lv_gba_emu_input_read_cb_t)(void* user_data);
typedef size_t (*lv_gba_emu_audio_output_cb_t)(void* user_data, const int16_t* data, size_t frames);

lv_obj_t* lv_gba_emu_create(lv_obj_t* par, const char* rom_file_path, lv_gba_view_mode_t mode);
void lv_gba_emu_del(lv_obj_t* gba_emu);
void lv_gba_emu_add_input_read_cb(lv_obj_t* gba_emu, lv_gba_emu_input_read_cb_t read_cb, void* user_data);
int lv_gba_emu_get_audio_sample_rate(lv_obj_t* gba_emu);
void lv_gba_emu_set_audio_output_cb(lv_obj_t* gba_emu, lv_gba_emu_audio_output_cb_t audio_output_cb, void* user_data);
void* lv_gba_emu_get_audio_output_user_data(lv_obj_t* gba_emu);

#ifdef __cplusplus
}
#endif

#endif
