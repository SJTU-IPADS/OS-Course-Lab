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
#include "lv_mem_macro.h"

struct gba_view_s {
    lv_obj_t* root;

    struct {
        lv_obj_t* canvas;
        lv_color_t* buf;
        bool is_buf_allocated;
    } screen;

    struct {
        struct {
            lv_obj_t* cont;
            lv_obj_t* up;
            lv_obj_t* down;
            lv_obj_t* left;
            lv_obj_t* right;
        } dir;

        struct {
            lv_obj_t* cont;
            lv_obj_t* A;
            lv_obj_t* B;
            lv_obj_t* L;
            lv_obj_t* R;
        } func;

        struct {
            lv_obj_t* cont;
            lv_obj_t* start;
            lv_obj_t* select;
        } ctrl;
    } btn;
};

typedef struct {
    const char* txt;
    lv_align_t align;
} btn_map_t;

static const btn_map_t btn_dir_map[] = {
    { LV_SYMBOL_UP, LV_ALIGN_TOP_MID },
    { LV_SYMBOL_DOWN, LV_ALIGN_BOTTOM_MID },
    { LV_SYMBOL_LEFT, LV_ALIGN_LEFT_MID },
    { LV_SYMBOL_RIGHT, LV_ALIGN_RIGHT_MID },
};

static const btn_map_t btn_func_map[] = {
    { "A", LV_ALIGN_LEFT_MID },
    { "B", LV_ALIGN_TOP_MID },
    { "L", LV_ALIGN_BOTTOM_MID },
    { "R", LV_ALIGN_RIGHT_MID },
};

static const btn_map_t btn_ctrl_map[] = {
    { "START", LV_ALIGN_LEFT_MID },
    { "SELECT", LV_ALIGN_RIGHT_MID },
};

static bool screen_create(gba_context_t* ctx)
{
    gba_view_t* view = ctx->view;

    lv_obj_t* canvas = lv_canvas_create(view->root);
    view->screen.canvas = canvas;

#if (LV_COLOR_DEPTH != 16)
    lv_coord_t width = ctx->av_info.fb_width;
    lv_coord_t height = ctx->av_info.fb_height;
    size_t buf_size = LV_IMG_BUF_SIZE_TRUE_COLOR(width, height);
    LV_ASSERT(buf_size > 0);

    view->screen.buf = lv_malloc(buf_size);
    LV_ASSERT_MALLOC(view->screen.buf);

    if (!view->screen.buf) {
        LV_LOG_ERROR("canvas buffer malloc failed");
        return false;
    }
    view->screen.is_buf_allocated = true;
    lv_canvas_set_buffer(view->screen.canvas, view->screen.buf, width, height, LV_IMG_CF_TRUE_COLOR);
#endif
    return true;
}

static uint32_t btn_read_cb(void* user_data)
{
    gba_view_t* view = user_data;

    uint32_t key_state = 0;

#define BTN_STATE_DEF(obj, joypad_id)                  \
    do {                                               \
        if (lv_obj_has_state(obj, LV_STATE_PRESSED)) { \
            key_state |= 1 << joypad_id;               \
        }                                              \
    } while (0)

    BTN_STATE_DEF(view->btn.dir.up, GBA_JOYPAD_UP);
    BTN_STATE_DEF(view->btn.dir.down, GBA_JOYPAD_DOWN);
    BTN_STATE_DEF(view->btn.dir.left, GBA_JOYPAD_LEFT);
    BTN_STATE_DEF(view->btn.dir.right, GBA_JOYPAD_RIGHT);

    BTN_STATE_DEF(view->btn.func.A, GBA_JOYPAD_A);
    BTN_STATE_DEF(view->btn.func.B, GBA_JOYPAD_B);
    BTN_STATE_DEF(view->btn.func.L, GBA_JOYPAD_L);
    BTN_STATE_DEF(view->btn.func.R, GBA_JOYPAD_R);

    BTN_STATE_DEF(view->btn.ctrl.select, GBA_JOYPAD_SELECT);
    BTN_STATE_DEF(view->btn.ctrl.start, GBA_JOYPAD_START);

    return key_state;
}

static void btn_create(gba_context_t* ctx)
{
    const lv_coord_t cont_size = 110;
    gba_view_t* view = ctx->view;
    {
        lv_obj_t* cont = lv_obj_create(view->root);

        view->btn.dir.cont = cont;

        lv_obj_set_size(cont, cont_size, cont_size);
        lv_obj_add_flag(cont, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
        lv_obj_set_style_pad_all(cont, 0, 0);

        lv_obj_t** btn_arr = &view->btn.dir.up;
        for (int i = 0; i < GBA_ARRAY_SIZE(btn_dir_map); i++) {
            lv_obj_t* btn = lv_btn_create(cont);
            btn_arr[i] = btn;
            lv_obj_align(btn, btn_dir_map[i].align, 0, 0);

            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_text(label, btn_dir_map[i].txt);
            lv_obj_center(label);
        }
    }

    {
        lv_obj_t* cont = lv_obj_create(view->root);

        view->btn.func.cont = cont;

        lv_obj_set_size(cont, cont_size, cont_size);
        lv_obj_set_style_pad_all(cont, 0, 0);

        lv_obj_t** btn_arr = &view->btn.func.A;
        for (int i = 0; i < GBA_ARRAY_SIZE(btn_func_map); i++) {
            lv_obj_t* btn = lv_btn_create(cont);
            btn_arr[i] = btn;
            lv_obj_align(btn, btn_func_map[i].align, 0, 0);

            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_text(label, btn_func_map[i].txt);
            lv_obj_center(label);
        }
    }

    {
        lv_obj_t* cont = lv_obj_create(view->root);

        view->btn.ctrl.cont = cont;

        lv_obj_set_size(cont, cont_size * 2, LV_SIZE_CONTENT);
        lv_obj_add_flag(cont, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
        lv_obj_set_style_pad_all(cont, 0, 0);

        lv_obj_t** btn_arr = &view->btn.ctrl.start;
        for (int i = 0; i < GBA_ARRAY_SIZE(btn_ctrl_map); i++) {
            lv_obj_t* btn = lv_btn_create(cont);
            btn_arr[i] = btn;
            lv_obj_align(btn, btn_ctrl_map[i].align, 0, 0);

            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_text(label, btn_ctrl_map[i].txt);
            lv_obj_center(label);
        }
    }

    lv_gba_emu_add_input_read_cb(view->root, btn_read_cb, view);
}

bool gba_view_init(gba_context_t* ctx, lv_obj_t* par, int mode)
{
    gba_view_t* view = lv_malloc(sizeof(gba_view_t));
    LV_ASSERT_MALLOC(view);
    ctx->view = view;

    lv_obj_t* root = lv_obj_create(par);
    {
        view->root = root;

        if (mode == LV_GBA_VIEW_MODE_SIMPLE) {
            lv_obj_remove_style_all(root);
        }

        lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_user_data(root, ctx);
        lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_pad_all(root, 20, 0);
        lv_obj_set_flex_align(root, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_AROUND);
    }

    bool retval = screen_create(ctx);

    if (mode != LV_GBA_VIEW_MODE_SIMPLE) {
        lv_obj_t* canvas = view->screen.canvas;
        lv_obj_set_style_outline_color(canvas, lv_theme_get_color_primary(canvas), 0);
        lv_obj_set_style_outline_width(canvas, 5, 0);
    }

    if (mode == LV_GBA_VIEW_MODE_VIRTUAL_KEYPAD) {
        btn_create(ctx);
    }

    return retval;
}

void gba_view_deinit(gba_context_t* ctx)
{
    LV_ASSERT_NULL(ctx);
    LV_ASSERT_NULL(ctx->view);
    if (ctx->view->screen.is_buf_allocated) {
        lv_free(ctx->view->screen.buf);
    }
    lv_free(ctx->view);
}

lv_obj_t* gba_view_get_root(gba_context_t* ctx)
{
    LV_ASSERT_NULL(ctx);
    LV_ASSERT_NULL(ctx->view);
    return ctx->view->root;
}

void gba_view_invalidate_frame(gba_context_t* ctx)
{
    LV_ASSERT_NULL(ctx);
    LV_ASSERT_NULL(ctx->view);
    lv_obj_invalidate(ctx->view->screen.canvas);
}

void gba_view_draw_frame(gba_context_t* ctx, const uint16_t* buf, lv_coord_t width, lv_coord_t height)
{
    lv_obj_t* canvas = ctx->view->screen.canvas;
#if (LV_COLOR_DEPTH == 16)
    if (ctx->view->screen.buf != (lv_color_t*)buf) {
        ctx->view->screen.buf = (lv_color_t*)buf;
        lv_canvas_set_buffer(canvas, ctx->view->screen.buf, ctx->av_info.fb_stride, height, LV_IMG_CF_TRUE_COLOR);
        lv_obj_set_width(canvas, width);
        LV_LOG_USER("set direct canvas buffer = %p", ctx->view->screen.buf);
    }
#else
    const lv_color16_t* src = (const lv_color16_t*)buf;
    lv_color_t* dst = ctx->view->screen.buf;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            *dst = lv_color_make(src->ch.red << 3, src->ch.green << 2, src->ch.blue << 3);
            dst++;
            src++;
        }
        src += (ctx->av_info.fb_stride - width);
    }
#endif

#if THREADED_RENDERER
    ctx->invalidate = true;
#else
    gba_view_invalidate_frame(ctx);
#endif
}
