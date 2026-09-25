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
 * Font header file.
 */
#pragma once

#include "g2d_internal.h"

PFONT _create_font(const char *fontbits_src, int byte_w, int byte_h,
                   int byte_reverse);
void _del_font(PFONT font);
int _draw_char(const PDC dc, char ch, int x, int y);
PFONT _get_default_bitfont(void);

const char *font_bits(const PFONT font);
int font_byte_width(const PFONT font);
int font_byte_height(const PFONT font);
int font_byte_size(const PFONT font);
int font_byte_reverse(const PFONT font);
