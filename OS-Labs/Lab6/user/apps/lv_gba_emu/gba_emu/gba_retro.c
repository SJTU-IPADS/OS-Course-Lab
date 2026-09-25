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
#include "gba_internal.h"
#include "libretro.h"
#include <math.h>
#include "lv_mem_macro.h"

#define GBA_FB_STRIDE 256
#ifndef GBA_FRAME_SKIP
#define GBA_FRAME_SKIP "0"
#endif

static gba_context_t* gba_ctx_p = NULL;

static void retro_log_printf_cb(enum retro_log_level level, const char* fmt, ...)
{
    static const char* prompt[] = {
        "DEBUG", "INFO", "WARN", "ERROR"
    };

    if (level >= GBA_ARRAY_SIZE(prompt)) {
        return;
    }

    char buffer[128];

    va_list ap;
    va_start(ap, fmt);
    lv_vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    LV_LOG("[RETRO][%s]: %s", prompt[level], buffer);
}

static bool retro_environment_cb(unsigned cmd, void* data)
{
    bool retval = true;
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
        struct retro_log_callback* log = data;
        log->log = retro_log_printf_cb;
        break;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        struct retro_variable* var = data;
        LV_LOG_USER("GET_VARIABLE: %s", var->key);
        if (strcmp(var->key, "vbanext_frameskip") == 0) {
            var->value = GBA_FRAME_SKIP;
        }
        break;
    }
    case RETRO_ENVIRONMENT_SET_MEMORY_MAPS: {
        struct retro_memory_map* mmaps = data;
        const struct retro_memory_descriptor* descs = mmaps->descriptors;
        for (int i = 0; i < mmaps->num_descriptors; i++) {
            LV_LOG_USER(
                "SET_MEMORY_MAPS: "
                "address space = %s\tptr = %p\tstart = %zx\tlen = %zu",
                descs->addrspace, descs->ptr, descs->start, descs->len);
            descs++;
        }
        break;
    }
    default:
        retval = false;
        break;
    }
    return retval;
}

static void retro_video_refresh_cb(const void* data, unsigned width, unsigned height, size_t pitch)
{
    gba_view_draw_frame(gba_ctx_p, data, width, height);
}

static void retro_audio_sample_cb(int16_t left, int16_t right)
{
}

static size_t retro_audio_sample_batch_cb(const int16_t* data, size_t frames)
{
    if (!gba_ctx_p->audio_output_cb) {
        return 0;
    }
    return gba_ctx_p->audio_output_cb(gba_ctx_p->audio_output_user_data, data, frames);
}

static void retro_input_poll_cb(void)
{
    gba_ctx_p->key_state = 0;
    gba_input_event_t* input_event;
    _LV_LL_READ(&gba_ctx_p->input_event_ll, input_event)
    {
        uint32_t key_state = input_event->read_cb(input_event->user_data);
        gba_ctx_p->key_state |= key_state;
    }
}

static int16_t retro_input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    return gba_ctx_p->key_state & (1 << id);
}

void gba_retro_init(gba_context_t* ctx)
{
    LV_ASSERT_MSG(gba_ctx_p == NULL, "Multi-instance mode is not supported");
    gba_ctx_p = ctx;
    retro_set_environment(retro_environment_cb);
    retro_set_video_refresh(retro_video_refresh_cb);
    retro_set_audio_sample(retro_audio_sample_cb);
    retro_set_audio_sample_batch(retro_audio_sample_batch_cb);
    retro_set_input_poll(retro_input_poll_cb);
    retro_set_input_state(retro_input_state_cb);
    retro_init();

    struct retro_system_av_info av_info;
    retro_get_system_av_info(&av_info);
    ctx->av_info.fb_width = av_info.geometry.base_width;
    ctx->av_info.fb_height = av_info.geometry.base_height;
    ctx->av_info.fb_stride = GBA_FB_STRIDE;
    ctx->av_info.fps = av_info.timing.fps;
    ctx->av_info.sample_rate = av_info.timing.sample_rate;
}

void gba_retro_deinit(gba_context_t* ctx)
{
    LV_ASSERT_NULL(gba_ctx_p);
    retro_unload_game();
    retro_deinit();
    gba_ctx_p = NULL;
}

bool gba_retro_load_game(gba_context_t* ctx, const char* path)
{
    struct retro_game_info info;
    lv_memset_00(&info, sizeof(info));
    info.path = path;
    return retro_load_game(&info);
}

void gba_retro_run(gba_context_t* ctx)
{
    retro_run();
#if THREADED_RENDERER
    if (ctx->invalidate) {
        gba_view_invalidate_frame(ctx);
        ctx->invalidate = false;
    }
#endif
}
