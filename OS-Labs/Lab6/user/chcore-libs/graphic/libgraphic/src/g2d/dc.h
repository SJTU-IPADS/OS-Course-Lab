/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

/*
 * Device context header file.
 */
#pragma once

#include "base_gfx.h"
#include "g2d_internal.h"

PDC _create_dc(enum DC_TYPE type);
void _del_dc(PDC dc);

u8 *get_dc_wr_ptr(const PDC dc, int x, int y);
u8 *get_dc_start_ptr(const PDC dc);
u8 *get_dc_end_ptr(const PDC dc);
u8 *get_dc_line_left_ptr(const PDC dc, int y);
u8 *get_dc_line_right_ptr(const PDC dc, int y);
int _get_dc_pmo_cap(const PDC dc);

void _dc_select_bitmap(PDC dc, const PBITMAP bitmap);
void _dc_select_clip_rgn(PDC dc, PRGN region);
void _dc_select_font(PDC dc, PFONT font);
void _dc_release_clip_rgn(PDC dc);

void _set_dc_fill_brush(PDC dc, PBRUSH brush);
void _set_dc_paint_brush(PDC dc, PBRUSH brush);
void _set_dc_txt_brush(PDC dc, PBRUSH brush);
void _set_dc_bg_brush(PDC dc, PBRUSH brush);

void _set_dc_fill_color(PDC dc, RGBA color);
void _set_dc_paint_color(PDC dc, RGBA color);
void _set_dc_txt_color(PDC dc, RGBA color);
void _set_dc_bg_color(PDC dc, RGBA color);

bool pt_in_dc(const PDC dc, int x, int y);

enum DC_TYPE dc_type(const PDC dc);
PBITMAP dc_bitmap(const PDC dc);
int dc_width(const PDC dc);
int dc_height(const PDC dc);
int dc_depth(const PDC dc);
int dc_pitch(const PDC dc);
int dc_pmo_cap(const PDC dc);
u8 *dc_raw_image(const PDC dc);
PRGN dc_clip_rgn(const PDC dc);

PFONT dc_font(const PDC dc);
int dc_font_byte_width(const PDC dc);
int dc_font_byte_height(const PDC dc);
int dc_font_byte_size(const PDC dc);
const char *dc_fontbits(const PDC dc);
int dc_font_bit_width(const PDC dc);
int dc_font_bit_height(const PDC dc);

PBRUSH dc_fill_brush(const PDC dc);
PBRUSH dc_paint_brush(const PDC dc);
PBRUSH dc_txt_brush(const PDC dc);
PBRUSH dc_bg_brush(const PDC dc);
RGBA dc_fill_color(const PDC dc);
RGBA dc_paint_color(const PDC dc);
RGBA dc_txt_color(const PDC dc);
RGBA dc_bg_color(const PDC dc);

const struct base_gfx *dc_gfx(const PDC dc);

void *next_line(const PDC dc, void *ptr);
void *prev_line(const PDC dc, void *ptr);
