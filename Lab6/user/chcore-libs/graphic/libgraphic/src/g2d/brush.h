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
 * Brush header file.
 */
#pragma once

#include "g2d_internal.h"

struct brush {
        RGBA color;
        u32 width;
};

PBRUSH _create_brush(void);
void _del_brush(PBRUSH brush);

const RGBA *brush_color(const PBRUSH brush);
u32 brush_width(const PBRUSH brush);

void set_brush_color(PBRUSH brush, RGBA color);
void set_brush_width(PBRUSH brush, u32 width);
