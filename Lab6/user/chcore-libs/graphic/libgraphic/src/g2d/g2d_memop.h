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
 * Memory operations header file.
 */
#pragma once

#define template_memop(type, dst, src, w, op, arg)   \
        {                                            \
                type *_dst = (type *)dst;            \
                const type *_src = (type *)src;      \
                int _i;                              \
                int _w0 = w / 4;                     \
                for (_i = 0; _i < _w0; ++_i) {       \
                        op(_dst, _src, arg);         \
                        op(_dst + 1, _src + 1, arg); \
                        op(_dst + 2, _src + 2, arg); \
                        op(_dst + 3, _src + 3, arg); \
                        _dst += 4;                   \
                        _src += 4;                   \
                }                                    \
                switch (w % 4) {                     \
                case 3:                              \
                        op(_dst, _src, arg);         \
                        op(_dst + 1, _src + 1, arg); \
                        op(_dst + 2, _src + 2, arg); \
                        break;                       \
                case 2:                              \
                        op(_dst, _src, arg);         \
                        op(_dst + 1, _src + 1, arg); \
                        break;                       \
                case 1:                              \
                        op(_dst, _src, arg);         \
                        break;                       \
                default:                             \
                        break;                       \
                }                                    \
        }

void dev_memand(void *restrict dst, const void *restrict src, int n);
void dev_memcpy(void *restrict dst, const void *restrict src, int n);
void dev_memerase(void *restrict dst, const void *restrict src, int n);
void dev_memxor(void *restrict dst, const void *restrict src, int n);
void dev_memor(void *restrict dst, const void *restrict src, int n);

void com_memand(void *restrict dst, const void *restrict src, int n);
void com_memcpy(void *restrict dst, const void *restrict src, int n);
void com_memerase(void *restrict dst, const void *restrict src, int n);
void com_memxor(void *restrict dst, const void *restrict src, int n);
void com_memor(void *restrict dst, const void *restrict src, int n);
