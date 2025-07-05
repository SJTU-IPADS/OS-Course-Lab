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
 * Implementation of pixel functions.
 */
#include "pixel.h"

#include "dc.h"

int _draw_pixel(const PDC dc, int x, int y)
{
        if (!pt_in_dc(dc, x, y))
                return -1;
        dc_gfx(dc)->draw_pixel(dc, x, y);
        return 0;
}

int _draw_pixels(const PDC dc, const POINT *points, int nr_points)
{
        int i;
        for (i = 0; i < nr_points; ++i) {
                if (!pt_in_dc(dc, points[i].x, points[i].y))
                        continue;
                dc_gfx(dc)->draw_pixel(dc, points[i].x, points[i].y);
        }
        return 0;
}
