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

#pragma once

#include "dc.h"
#include "g2d_memop.h"

/*
 * bitblt
 */

#define template_bitblt(type, dst_dc, x1, y1, src_dc, x2, y2, w, h, op, arg) \
        {                                                                    \
                type *_dst_ptr = (type *)get_dc_wr_ptr(dst_dc, x1, y1);      \
                type *_src_ptr = (type *)get_dc_wr_ptr(src_dc, x2, y2);      \
                int _y;                                                      \
                for (_y = 0; _y < h; ++_y) {                                 \
                        template_memop(type, _dst_ptr, _src_ptr, w, op, arg);        \
                        _dst_ptr = next_line(dst_dc, _dst_ptr);              \
                        _src_ptr = next_line(src_dc, _src_ptr);              \
                }                                                            \
        }

/*
 * stretch_blt
 */

/*
 * w1 >= w2
 * : sx1 < sx2
 */
#define template_strecth_line_1(                                            \
        type, _dst_ptr, sx1, _src_ptr, sx2, _src_xend_ptr, px, pw, op, arg) \
        {                                                                   \
                type *_dst = _dst_ptr;                                      \
                type *_src = _src_ptr;                                      \
                int _qx1 = px;                                              \
                int _qx2 = px;                                              \
                while (_qx1 < pw) {                                         \
                        if (_src >= _src_xend_ptr)                          \
                                break;                                      \
                        op(_dst, _src, arg);                                \
                        _qx1 += sx1;                                        \
                        ++_dst;                                             \
                        if (_qx1 > _qx2) {                                  \
                                _qx2 += sx2;                                \
                                ++_src;                                     \
                        }                                                   \
                }                                                           \
        }

/*
 * w1 < w2
 * : sx1 >= sx2
 */
#define template_strecth_line_2(                                         \
        type, dst_ptr, sx1, src_ptr, sx2, src_xend_ptr, px, pw, op, arg) \
        {                                                                \
                type *_dst = dst_ptr;                                    \
                type *_src = src_ptr;                                    \
                int _qx1 = px;                                           \
                int _qx2 = px;                                           \
                while (_qx2 < pw) {                                      \
                        if (_src >= src_xend_ptr)                        \
                                break;                                   \
                        if (_qx2 >= _qx1) {                              \
                                op(_dst, _src, arg);                     \
                                _qx1 += sx1;                             \
                                ++_dst;                                  \
                        }                                                \
                        _qx2 += sx2;                                     \
                        ++_src;                                          \
                }                                                        \
        }

/*
 * h1 >= h2, w1 >= w2
 * : sy1 < sy2, sx1 < sx2
 */
#define template_strecth_rect_1(type,                                     \
                                dst_dc,                                   \
                                x1,                                       \
                                y1,                                       \
                                sx1,                                      \
                                sy1,                                      \
                                src_dc,                                   \
                                x2,                                       \
                                y2,                                       \
                                sx2,                                      \
                                sy2,                                      \
                                px,                                       \
                                py,                                       \
                                pw,                                       \
                                ph,                                       \
                                op,                                       \
                                arg)                                      \
        {                                                                 \
                type *_dst_ptr = (type *)get_dc_wr_ptr(dst_dc, x1, y1);   \
                type *_src_ptr = (type *)get_dc_wr_ptr(src_dc, x2, y2);   \
                type *_src_xend_ptr =                                     \
                        (type *)get_dc_line_right_ptr(src_dc, y2);        \
                type *_src_yend_ptr = (type *)get_dc_end_ptr(src_dc);     \
                int _qy1 = py;                                            \
                int _qy2 = py;                                            \
                while (_qy1 < ph) {                                       \
                        if (_src_ptr >= _src_yend_ptr)                    \
                                break;                                    \
                        template_strecth_line_1(type,                     \
                                                _dst_ptr,                 \
                                                sx1,                      \
                                                _src_ptr,                 \
                                                sx2,                      \
                                                _src_xend_ptr,            \
                                                px,                       \
                                                pw,                       \
                                                op,                       \
                                                arg);                     \
                        _qy1 += sy1;                                      \
                        _dst_ptr = next_line(dst_dc, _dst_ptr);           \
                        if (_qy1 > _qy2) {                                \
                                _qy2 += sy2;                              \
                                _src_ptr = next_line(src_dc, _src_ptr);   \
                                _src_xend_ptr =                           \
                                        next_line(src_dc, _src_xend_ptr); \
                        }                                                 \
                }                                                         \
        }

/*
 * h1 >= h2, w1 < w2
 * : sy1 < sy2, sx1 >= sx2
 */
#define template_strecth_rect_2(type,                                     \
                                dst_dc,                                   \
                                x1,                                       \
                                y1,                                       \
                                sx1,                                      \
                                sy1,                                      \
                                src_dc,                                   \
                                x2,                                       \
                                y2,                                       \
                                sx2,                                      \
                                sy2,                                      \
                                px,                                       \
                                py,                                       \
                                pw,                                       \
                                ph,                                       \
                                op,                                       \
                                arg)                                      \
        {                                                                 \
                type *_dst_ptr = (type *)get_dc_wr_ptr(dst_dc, x1, y1);   \
                type *_src_ptr = (type *)get_dc_wr_ptr(src_dc, x2, y2);   \
                type *_src_xend_ptr =                                     \
                        (type *)get_dc_line_right_ptr(src_dc, y2);        \
                type *_src_yend_ptr = (type *)get_dc_end_ptr(src_dc);     \
                int _qy1 = py;                                            \
                int _qy2 = py;                                            \
                while (_qy1 < ph) {                                       \
                        if (_src_ptr >= _src_yend_ptr)                    \
                                break;                                    \
                        template_strecth_line_2(type,                     \
                                                _dst_ptr,                 \
                                                sx1,                      \
                                                _src_ptr,                 \
                                                sx2,                      \
                                                _src_xend_ptr,            \
                                                px,                       \
                                                pw,                       \
                                                op,                       \
                                                arg);                     \
                        _qy1 += sy1;                                      \
                        _dst_ptr = next_line(dst_dc, _dst_ptr);           \
                        if (_qy1 > _qy2) {                                \
                                _qy2 += sy2;                              \
                                _src_ptr = next_line(src_dc, _src_ptr);   \
                                _src_xend_ptr =                           \
                                        next_line(src_dc, _src_xend_ptr); \
                        }                                                 \
                }                                                         \
        }

/*
 * h1 < h2, w1 >= w2
 * : sy1 >= sy2, sx1 < sx2
 */
#define template_strecth_rect_3(type,                                     \
                                dst_dc,                                   \
                                x1,                                       \
                                y1,                                       \
                                sx1,                                      \
                                sy1,                                      \
                                src_dc,                                   \
                                x2,                                       \
                                y2,                                       \
                                sx2,                                      \
                                sy2,                                      \
                                px,                                       \
                                py,                                       \
                                pw,                                       \
                                ph,                                       \
                                op,                                       \
                                arg)                                      \
        {                                                                 \
                type *_dst_ptr = (type *)get_dc_wr_ptr(dst_dc, x1, y1);   \
                type *_src_ptr = (type *)get_dc_wr_ptr(src_dc, x2, y2);   \
                type *_src_xend_ptr =                                     \
                        (type *)get_dc_line_right_ptr(src_dc, y2);        \
                type *_src_yend_ptr = (type *)get_dc_end_ptr(src_dc);     \
                int _qy1 = py;                                            \
                int _qy2 = py;                                            \
                while (_qy2 < ph) {                                       \
                        if (_src_ptr >= _src_yend_ptr)                    \
                                break;                                    \
                        if (_qy2 >= _qy1) {                               \
                                template_strecth_line_1(type,             \
                                                        _dst_ptr,         \
                                                        sx1,              \
                                                        _src_ptr,         \
                                                        sx2,              \
                                                        _src_xend_ptr,    \
                                                        px,               \
                                                        pw,               \
                                                        op,               \
                                                        arg);             \
                                _qy1 += sy1;                              \
                                _dst_ptr = next_line(dst_dc, _dst_ptr);   \
                        }                                                 \
                        _qy2 += sy2;                                      \
                        _src_ptr = next_line(src_dc, _src_ptr);           \
                        _src_xend_ptr = next_line(src_dc, _src_xend_ptr); \
                }                                                         \
        }

/*
 * h1 < h2, w1 < w2
 * : sy1 >= sy2, sx1 >= sx2
 */
#define template_strecth_rect_4(type,                                     \
                                dst_dc,                                   \
                                x1,                                       \
                                y1,                                       \
                                sx1,                                      \
                                sy1,                                      \
                                src_dc,                                   \
                                x2,                                       \
                                y2,                                       \
                                sx2,                                      \
                                sy2,                                      \
                                px,                                       \
                                py,                                       \
                                pw,                                       \
                                ph,                                       \
                                op,                                       \
                                arg)                                      \
        {                                                                 \
                type *_dst_ptr = (type *)get_dc_wr_ptr(dst_dc, x1, y1);   \
                type *_src_ptr = (type *)get_dc_wr_ptr(src_dc, x2, y2);   \
                type *_src_xend_ptr =                                     \
                        (type *)get_dc_line_right_ptr(src_dc, y2);        \
                type *_src_yend_ptr = (type *)get_dc_end_ptr(src_dc);     \
                int _qy1 = py;                                            \
                int _qy2 = py;                                            \
                while (_qy2 < ph) {                                       \
                        if (_src_ptr >= _src_yend_ptr)                    \
                                break;                                    \
                        if (_qy2 >= _qy1) {                               \
                                template_strecth_line_2(type,             \
                                                        _dst_ptr,         \
                                                        sx1,              \
                                                        _src_ptr,         \
                                                        sx2,              \
                                                        _src_xend_ptr,    \
                                                        px,               \
                                                        pw,               \
                                                        op,               \
                                                        arg);             \
                                _qy1 += sy1;                              \
                                _dst_ptr = next_line(dst_dc, _dst_ptr);   \
                        }                                                 \
                        _qy2 += sy2;                                      \
                        _src_ptr = next_line(src_dc, _src_ptr);           \
                        _src_xend_ptr = next_line(src_dc, _src_xend_ptr); \
                }                                                         \
        }

#define template_stretch_blt(type,                              \
                             dst_dc,                            \
                             x1,                                \
                             y1,                                \
                             sx1,                               \
                             sy1,                               \
                             src_dc,                            \
                             x2,                                \
                             y2,                                \
                             sx2,                               \
                             sy2,                               \
                             px,                                \
                             py,                                \
                             pw,                                \
                             ph,                                \
                             op,                                \
                             arg)                               \
        {                                                       \
                if ((sy1) < (sy2)) {                            \
                        if ((sx1) < (sx2)) {                    \
                                template_strecth_rect_1(type,   \
                                                        dst_dc, \
                                                        (x1),   \
                                                        (y1),   \
                                                        (sx1),  \
                                                        (sy1),  \
                                                        src_dc, \
                                                        (x2),   \
                                                        (y2),   \
                                                        (sx2),  \
                                                        (sy2),  \
                                                        (px),   \
                                                        (py),   \
                                                        (pw),   \
                                                        (ph),   \
                                                        op,     \
                                                        arg);   \
                        } else {                                \
                                template_strecth_rect_2(type,   \
                                                        dst_dc, \
                                                        (x1),   \
                                                        (y1),   \
                                                        (sx1),  \
                                                        (sy1),  \
                                                        src_dc, \
                                                        (x2),   \
                                                        (y2),   \
                                                        (sx2),  \
                                                        (sy2),  \
                                                        (px),   \
                                                        (py),   \
                                                        (pw),   \
                                                        (ph),   \
                                                        op,     \
                                                        arg);   \
                        }                                       \
                } else {                                        \
                        if ((sx1) < (sx2)) {                    \
                                template_strecth_rect_3(type,   \
                                                        dst_dc, \
                                                        (x1),   \
                                                        (y1),   \
                                                        (sx1),  \
                                                        (sy1),  \
                                                        src_dc, \
                                                        (x2),   \
                                                        (y2),   \
                                                        (sx2),  \
                                                        (sy2),  \
                                                        (px),   \
                                                        (py),   \
                                                        (pw),   \
                                                        (ph),   \
                                                        op,     \
                                                        arg);   \
                        } else {                                \
                                template_strecth_rect_4(type,   \
                                                        dst_dc, \
                                                        (x1),   \
                                                        (y1),   \
                                                        (sx1),  \
                                                        (sy1),  \
                                                        src_dc, \
                                                        (x2),   \
                                                        (y2),   \
                                                        (sx2),  \
                                                        (sy2),  \
                                                        (px),   \
                                                        (py),   \
                                                        (pw),   \
                                                        (ph),   \
                                                        op,     \
                                                        arg);   \
                        }                                       \
                }                                               \
        }

/*
 * Arc:
 * _xc: xCenter
 * _yc: yCenter
 * _a: xRadius
 * _b: yRadius
 */

/* Set pixel in 4 quadrants */
#define ellipse_plot_points(type, dc, _xc, _yc, _x, _y, color)              \
        {                                                                   \
                type *_ptr = (type *)get_dc_wr_ptr(dc, _xc + _x, _yc + _y); \
                int _tow_x = (_x) << 1;                                     \
                int _two_y = (_y) << 1;                                     \
                /* Quadrant I */                                            \
                if (pt_in_dc(dc, _xc + _x, _yc + _y))                       \
                        *_ptr = color;                                      \
                /* Quadrant II */                                           \
                _ptr -= _tow_x;                                             \
                if (pt_in_dc(dc, _xc - _x, _yc + _y))                       \
                        *_ptr = color;                                      \
                /* Quadrant III */                                          \
                _ptr = (type *)((u64)_ptr - _two_y * dc_pitch(dc));         \
                if (pt_in_dc(dc, _xc - _x, _yc - _y))                       \
                        *_ptr = color;                                      \
                /* Quadrant IV */                                           \
                _ptr += _tow_x;                                             \
                if (pt_in_dc(dc, _xc + _x, _yc - _y))                       \
                        *_ptr = color;                                      \
        }

/* Draw a whole ellipse */
#define template_draw_ellipse(type, dc, left, top, right, bottom, color)       \
        {                                                                      \
                int _xc = (left + right) >> 1;                                 \
                int _yc = (top + bottom) >> 1;                                 \
                int _a = (right - left) >> 1;                                  \
                int _b = (bottom - top) >> 1;                                  \
                                                                               \
                int _a2 = _a * _a;                                             \
                int _b2 = _b * _b;                                             \
                int _two_a2 = _a2 << 1;                                        \
                int _two_b2 = _b2 << 1;                                        \
                int _p;                                                        \
                int _x = 0;                                                    \
                int _y = _b;                                                   \
                int _px = 0;                                                   \
                int _py = _two_a2 * _b;                                        \
                                                                               \
                /* Region 1 */                                                 \
                ellipse_plot_points(type, dc, _xc, _yc, _x, _y, color);        \
                _p = _b2 - (_a2 * _b) + (_a2 >> 2);                            \
                while (_px < _py) {                                            \
                        ++_x;                                                  \
                        _px += _two_b2;                                        \
                        if (_p < 0) {                                          \
                                _p += _px + _b2;                               \
                        } else {                                               \
                                --_y;                                          \
                                _py -= _two_a2;                                \
                                _p += _px + _b2 - _py + _a2;                   \
                        }                                                      \
                        ellipse_plot_points(                                   \
                                type, dc, _xc, _yc, _x, _y, color);            \
                }                                                              \
                /* Region 2 */                                                 \
                _p = _b2 * (_x + 0.5) * (_x + 0.5) + _a2 * (_y - 1) * (_y - 1) \
                     - _a2 * _b2;                                              \
                while (_y > 0) {                                               \
                        --_y;                                                  \
                        _py -= _two_a2;                                        \
                        if (_p > 0) {                                          \
                                _p += _a2 - _py;                               \
                        } else {                                               \
                                ++_x;                                          \
                                _px += _two_b2;                                \
                                _p += _a2 - _py + _px + _b2;                   \
                        }                                                      \
                        ellipse_plot_points(                                   \
                                type, dc, _xc, _yc, _x, _y, color);            \
                }                                                              \
        }

/*
 * Horizontal line
 */

#define template_draw_hline(type, ptr_line, w0, w1, color) \
        {                                                  \
                type *_ptr = ptr_line;                     \
                int _x;                                    \
                for (_x = 0; _x < (w0); ++_x) {            \
                        _ptr[0] = color;                   \
                        _ptr[1] = color;                   \
                        _ptr[2] = color;                   \
                        _ptr[3] = color;                   \
                        _ptr += 4;                         \
                }                                          \
                switch (w1) {                              \
                case 3:                                    \
                        _ptr[0] = color;                   \
                        _ptr[1] = color;                   \
                        _ptr[2] = color;                   \
                        break;                             \
                case 2:                                    \
                        _ptr[0] = color;                   \
                        _ptr[1] = color;                   \
                        break;                             \
                case 1:                                    \
                        _ptr[0] = color;                   \
                        break;                             \
                default:                                   \
                        break;                             \
                }                                          \
        }

/*
 * Vertical line
 */

#define template_draw_vline(type, dc, ptr_line, h0, h1, color) \
        {                                                      \
                type *_ptr = ptr_line;                         \
                int _y;                                        \
                for (_y = 0; _y < (h0); ++_y) {                \
                        *_ptr = color;                         \
                        _ptr = next_line(dc, _ptr);            \
                        *_ptr = color;                         \
                        _ptr = next_line(dc, _ptr);            \
                        *_ptr = color;                         \
                        _ptr = next_line(dc, _ptr);            \
                        *_ptr = color;                         \
                        _ptr = next_line(dc, _ptr);            \
                }                                              \
                switch (h1) {                                  \
                case 3:                                        \
                        *_ptr = color;                         \
                        _ptr = next_line(dc, _ptr);            \
                        *_ptr = color;                         \
                        _ptr = next_line(dc, _ptr);            \
                        *_ptr = color;                         \
                        _ptr = next_line(dc, _ptr);            \
                        break;                                 \
                case 2:                                        \
                        *_ptr = color;                         \
                        _ptr = next_line(dc, _ptr);            \
                        *_ptr = color;                         \
                        _ptr = next_line(dc, _ptr);            \
                        break;                                 \
                case 1:                                        \
                        *_ptr = color;                         \
                        _ptr = next_line(dc, _ptr);            \
                        break;                                 \
                default:                                       \
                        break;                                 \
                }                                              \
        }

/*
 * Line:
 * _y = kx + b,
 * _x0 < x1
 */

/* 0 < k < 1 */
#define template_bres_draw_line_1(type, dc, x0, y0, x1, y1, color) \
        {                                                          \
                type *_ptr = (type *)get_dc_wr_ptr(dc, x0, y0);    \
                int _dx = x1 - x0;                                 \
                int _dy = y1 - y0;                                 \
                int _two_dy = _dy << 1;                            \
                int _two_dx = _dx << 1;                            \
                int _p = 0;                                        \
                int _x = x0, _y = y0;                              \
                                                                   \
                *_ptr = color;                                     \
                while (_x < x1) {                                  \
                        ++_x;                                      \
                        _p += _two_dy;                             \
                        if (_p >= _dx) {                           \
                                _p -= _two_dx;                     \
                                ++_y;                              \
                                _ptr = next_line(dc, _ptr);        \
                        }                                          \
                        ++_ptr;                                    \
                        *_ptr = color;                             \
                }                                                  \
        }

/* k >= 1 */
#define template_bres_draw_line_2(type, dc, x0, y0, x1, y1, color) \
        {                                                          \
                type *_ptr = (type *)get_dc_wr_ptr(dc, x0, y0);    \
                int _dx = x1 - x0;                                 \
                int _dy = y1 - y0;                                 \
                int _two_dy = _dy << 1;                            \
                int _two_dx = _dx << 1;                            \
                int _p = 0;                                        \
                int _x = x0, _y = y0;                              \
                                                                   \
                *_ptr = color;                                     \
                while (_y < y1) {                                  \
                        ++_y;                                      \
                        _p += _two_dx;                             \
                        if (_p >= _dy) {                           \
                                _p -= _two_dy;                     \
                                ++_x;                              \
                                ++_ptr;                            \
                        }                                          \
                        _ptr = next_line(dc, _ptr);                \
                        *_ptr = color;                             \
                }                                                  \
        }

/* -1 < k < 0 */
#define template_bres_draw_line_3(type, dc, x0, y0, x1, y1, color) \
        {                                                          \
                type *_ptr = (type *)get_dc_wr_ptr(dc, x0, y0);    \
                int _dx = x1 - x0;                                 \
                int _dy = y0 - y1;                                 \
                int _two_dy = _dy << 1;                            \
                int _two_dx = _dx << 1;                            \
                int _p = 0;                                        \
                int _x = x0, _y = y0;                              \
                                                                   \
                *_ptr = color;                                     \
                while (_x < x1) {                                  \
                        ++_x;                                      \
                        _p += _two_dy;                             \
                        if (_p >= _dx) {                           \
                                _p -= _two_dx;                     \
                                --_y;                              \
                                prev_line(dc, _ptr);               \
                        }                                          \
                        ++_ptr;                                    \
                        *_ptr = color;                             \
                }                                                  \
        }

/* k <= -1 */
#define template_bres_draw_line_4(type, dc, x0, y0, x1, y1, color) \
        {                                                          \
                type *_ptr = (type *)get_dc_wr_ptr(dc, x0, y0);    \
                int _dx = x1 - x0;                                 \
                int _dy = y0 - y1;                                 \
                int _two_dy = _dy << 1;                            \
                int _two_dx = _dx << 1;                            \
                int _p = 0;                                        \
                int _x = x0, _y = y0;                              \
                                                                   \
                *_ptr = color;                                     \
                while (_y > y1) {                                  \
                        --_y;                                      \
                        _p += _two_dx;                             \
                        if (_p >= _dy) {                           \
                                _p -= _two_dy;                     \
                                ++_x;                              \
                                ++_ptr;                            \
                        }                                          \
                        prev_line(dc, _ptr);                       \
                        *_ptr = color;                             \
                }                                                  \
        }

#define template_draw_line(type, dc, x1, y1, x2, y2, color)                   \
        {                                                                     \
                int _x0, _y0, _x3, _y3;                                       \
                if ((x1) < (x2)) {                                            \
                        _x0 = (x1);                                           \
                        _y0 = (y1);                                           \
                        _x3 = (x2);                                           \
                        _y3 = (y2);                                           \
                } else {                                                      \
                        _x0 = (x2);                                           \
                        _y0 = (y2);                                           \
                        _x3 = (x1);                                           \
                        _y3 = (y1);                                           \
                }                                                             \
                if (_y3 > _y0) {                                              \
                        if (_x3 - _x0 > _y3 - _y0) {                          \
                                template_bres_draw_line_1(                    \
                                        type, dc, _x0, _y0, _x3, _y3, color); \
                        } else {                                              \
                                template_bres_draw_line_2(                    \
                                        type, dc, _x0, _y0, _x3, _y3, color); \
                        }                                                     \
                } else {                                                      \
                        if (_x3 - _x0 > _y0 - _y3) {                          \
                                template_bres_draw_line_3(                    \
                                        type, dc, _x0, _y0, _x3, _y3, color); \
                        } else {                                              \
                                template_bres_draw_line_4(                    \
                                        type, dc, _x0, _y0, _x3, _y3, color); \
                        }                                                     \
                }                                                             \
        }

/*
 * Bit font
 */

#define template_draw_char(                                                \
        type, dc, ch, x, y, left, top, right, bottom, bg_color, txt_color) \
        {                                                                  \
                type *_ptr = (type *)get_dc_wr_ptr(dc, x + left, y + top); \
                int _row, _col, _i, _j;                                    \
                const char *_font_ptr;                                     \
                char _font_char;                                           \
                                                                           \
                _font_ptr = dc_fontbits(dc) + ch * dc_font_byte_size(dc)   \
                            + top * dc_font_byte_width(dc) + left / 8;     \
                for (_row = 0; _row <= bottom - top; ++_row) {             \
                        _i = left / 8;                                     \
                        _j = left % 8;                                     \
                        _font_char = (_font_ptr[_i]) << _j;                \
                        for (_col = 0; _col <= right - left; ++_col) {     \
                                if ((_font_char & 0x80) == 0) {            \
                                        _ptr[_col] = bg_color;             \
                                } else {                                   \
                                        _ptr[_col] = txt_color;            \
                                }                                          \
                                _font_char <<= 1;                          \
                                ++_j;                                      \
                                if (_j == 8) {                             \
                                        ++_i;                              \
                                        _j = 0;                            \
                                        _font_char = _font_ptr[_i];        \
                                }                                          \
                        }                                                  \
                        _ptr = next_line(dc, _ptr);                        \
                        _font_ptr += dc_font_byte_width(dc);               \
                }                                                          \
        }
