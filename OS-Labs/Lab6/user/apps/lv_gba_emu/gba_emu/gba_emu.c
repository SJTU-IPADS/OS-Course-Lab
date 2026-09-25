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
#include "gba_emu.h"
#include "gba_internal.h"
#include "lvgl/lvgl.h"

static void gba_context_init(gba_context_t* ctx)
{
    LV_ASSERT_NULL(ctx);
    lv_memset_00(ctx, sizeof(gba_context_t));
    _lv_ll_init(&ctx->input_event_ll, sizeof(gba_input_event_t));
}

static void gba_emu_timer_cb(lv_timer_t* timer)
{
    gba_context_t* gba_ctx = timer->user_data;
    gba_retro_run(gba_ctx);
}

static bool gba_emu_change_rom_size(const char* path)
{
    lv_fs_file_t file;
    lv_fs_res_t res = lv_fs_open(&file, path, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) {
        LV_LOG_ERROR("open %s failed: %d", path, res);
        return false;
    }

    lv_fs_seek(&file, 0, LV_FS_SEEK_END);

    uint32_t pos;
    res = lv_fs_tell(&file, &pos);
    lv_fs_close(&file);

    if (res != LV_FS_RES_OK) {
        LV_LOG_ERROR("get file size failed: %d", res);
        return false;
    }

    void gba_set_rom_size(int size);
    gba_set_rom_size(pos);
    LV_LOG_USER("ROM: %s size = %" LV_PRIu32 " Bytes", path, pos);
    return true;
}

lv_obj_t* lv_gba_emu_create(lv_obj_t* par, const char* rom_file_path, lv_gba_view_mode_t mode)
{
    LV_ASSERT_NULL(rom_file_path);

    gba_context_t* gba_ctx = lv_malloc(sizeof(gba_context_t));
    LV_ASSERT_MALLOC(gba_ctx);
    gba_context_init(gba_ctx);

    char real_path[512];
    lv_snprintf(real_path, sizeof(real_path), "%s", rom_file_path);

    if (!gba_emu_change_rom_size(real_path)) {
        return NULL;
    }

    gba_retro_init(gba_ctx);

    if (!gba_view_init(gba_ctx, par, mode)) {
        goto failed;
    }

    if (!gba_retro_load_game(gba_ctx, real_path)) {
        LV_LOG_ERROR("load ROM: %s failed", real_path);
        goto failed;
    }

    gba_ctx->timer = lv_timer_create(gba_emu_timer_cb, 1000 / gba_ctx->av_info.fps, gba_ctx);

failed:
    return gba_view_get_root(gba_ctx);
}

void lv_gba_emu_del(lv_obj_t* gba_emu)
{
    gba_context_t* gba_ctx = lv_obj_get_user_data(gba_emu);
    LV_ASSERT_NULL(gba_ctx);

    if (gba_ctx->timer) {
        lv_timer_del(gba_ctx->timer);
    }

    gba_view_deinit(gba_ctx);
    gba_retro_deinit(gba_ctx);
    _lv_ll_clear(&gba_ctx->input_event_ll);
    lv_free(gba_ctx);
    lv_obj_del(gba_emu);
}

void lv_gba_emu_add_input_read_cb(lv_obj_t* gba_emu, lv_gba_emu_input_read_cb_t read_cb, void* user_data)
{
    gba_context_t* ctx = lv_obj_get_user_data(gba_emu);
    LV_ASSERT_NULL(ctx);
    gba_input_event_t* input_event = _lv_ll_ins_tail(&ctx->input_event_ll);
    LV_ASSERT_MALLOC(input_event);
    input_event->read_cb = read_cb;
    input_event->user_data = user_data;
}

int lv_gba_emu_get_audio_sample_rate(lv_obj_t* gba_emu)
{
    gba_context_t* ctx = lv_obj_get_user_data(gba_emu);
    LV_ASSERT_NULL(ctx);
    return (int)ctx->av_info.sample_rate;
}

void lv_gba_emu_set_audio_output_cb(lv_obj_t* gba_emu, lv_gba_emu_audio_output_cb_t audio_output_cb, void* user_data)
{
    gba_context_t* ctx = lv_obj_get_user_data(gba_emu);
    LV_ASSERT_NULL(ctx);
    ctx->audio_output_cb = audio_output_cb;
    ctx->audio_output_user_data = user_data;
}

void* lv_gba_emu_get_audio_output_user_data(lv_obj_t* gba_emu)
{
        gba_context_t* ctx = lv_obj_get_user_data(gba_emu);
        LV_ASSERT_NULL(ctx);
        return ctx->audio_output_user_data;
}
