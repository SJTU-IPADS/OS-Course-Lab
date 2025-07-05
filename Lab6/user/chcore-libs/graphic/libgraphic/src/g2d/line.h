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
 * Line header file.
 */
#pragma once

#include "g2d_internal.h"

int _draw_hline(const PDC dc, int l, int r, int y);
int _draw_vline(const PDC dc, int x, int t, int b);
int _draw_line(const PDC dc, int x1, int y1, int x2, int y2);
int _draw_rect(const PDC dc, int l, int t, int r, int b);
int _fill_rect(const PDC dc, int l, int t, int r, int b);
