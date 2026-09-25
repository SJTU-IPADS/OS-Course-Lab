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
 * Blend header file.
 */
#pragma once

#include "g2d_internal.h"

void blend_pixel_16(RGB565 *dst, const RGB565 *src, u8 alpha);
void blend_pixel_24(RGB *dst, const RGB *src, u8 alpha);
void blend_pixel_32(RGBA *dst, const RGBA *src, u8 alpha);
void blend_pixel_rgba(RGBA *dst, const RGBA *src, u8 alpha);

int _blend(PDC dst, int x1, int y1, int w1, int h1, PDC src, int x2, int y2,
           int w2, int h2, u8 alpha);
int _blend_rgba(PDC dst, int x1, int y1, int w1, int h1, PDC src, int x2,
                int y2, int w2, int h2, u8 alpha);
