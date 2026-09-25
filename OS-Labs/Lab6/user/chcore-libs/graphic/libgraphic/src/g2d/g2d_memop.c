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
 * Implementation of 128-bit memory operation functions.
 */
#include "g2d_memop.h"

#include "../dep/os.h"

#define ALIGN_64bit 8

/* 64-bit align */
#define template_memop64_head(dst, src, op, w)                  \
        {                                                       \
                switch (w) {                                    \
                case 7:                                         \
                        *(u32 *)(dst - 4) op *(u32 *)(src - 4); \
                        *(u16 *)(dst - 6) op *(u16 *)(src - 6); \
                        *(u8 *)(dst - 7) op *(u8 *)(src - 7);   \
                        break;                                  \
                case 6:                                         \
                        *(u32 *)(dst - 4) op *(u32 *)(src - 4); \
                        *(u16 *)(dst - 6) op *(u16 *)(src - 6); \
                        break;                                  \
                case 5:                                         \
                        *(u32 *)(dst - 4) op *(u32 *)(src - 4); \
                        *(u8 *)(dst - 5) op *(u8 *)(src - 5);   \
                        break;                                  \
                case 4:                                         \
                        *(u32 *)(dst - 4) op *(u32 *)(src - 4); \
                        break;                                  \
                case 3:                                         \
                        *(u16 *)(dst - 2) op *(u16 *)(src - 2); \
                        *(u8 *)(dst - 3) op *(u8 *)(src - 3);   \
                        break;                                  \
                case 2:                                         \
                        *(u16 *)(dst - 2) op *(u16 *)(src - 2); \
                        break;                                  \
                case 1:                                         \
                        *(u8 *)(dst - 1) op *(u8 *)(src - 1);   \
                        break;                                  \
                default:                                        \
                        break;                                  \
                }                                               \
        }

/* 64-bit align */
#define template_memop64_tail(dst, src, op, w)                  \
        {                                                       \
                switch (w) {                                    \
                case 7:                                         \
                        *(u32 *)(dst)op *(u32 *)(src);          \
                        *(u16 *)(dst + 4) op *(u16 *)(src + 4); \
                        *(u8 *)(dst + 6) op *(u8 *)(src + 6);   \
                        break;                                  \
                case 6:                                         \
                        *(u32 *)(dst)op *(u32 *)(src);          \
                        *(u16 *)(dst + 4) op *(u16 *)(src + 4); \
                        break;                                  \
                case 5:                                         \
                        *(u32 *)(dst)op *(u32 *)(src);          \
                        *(u8 *)(dst + 4) op *(u8 *)(src + 4);   \
                        break;                                  \
                case 4:                                         \
                        *(u32 *)(dst)op *(u32 *)(src);          \
                        break;                                  \
                case 3:                                         \
                        *(u16 *)(dst)op *(u16 *)(src);          \
                        *(u8 *)(dst + 2) op *(u8 *)(src + 2);   \
                        break;                                  \
                case 2:                                         \
                        *(u16 *)(dst)op *(u16 *)(src);          \
                        break;                                  \
                case 1:                                         \
                        *(u8 *)(dst)op *(u8 *)(src);            \
                        break;                                  \
                default:                                        \
                        break;                                  \
                }                                               \
        }

/* For short n at 64-bit align */
#define template_memop64_short(dst, src, op, n)                      \
        {                                                            \
                volatile int _j;                                     \
                if (((u64)dst) % 4 == 0) {                           \
                        template_memop64_tail(dst, src, op, n);      \
                } else {                                             \
                        for (_j = 0; _j < n / 2; ++_j) {             \
                                ((u16 *)dst)[_j] op((u16 *)src)[_j]; \
                        }                                            \
                }                                                    \
        }

/* AND */
static inline void memand_64(u64 *dst, const u64 *src, void *arg)
{
        *dst &= *src;
}

/* COPY */
static inline void memcpy_64(u64 *dst, const u64 *src, void *arg)
{
        *dst = *src;
}

/* INVERT + AND */
static inline void meminvertand_64(u64 *dst, const u64 *src, void *arg)
{
        *dst = ~*dst;
        *dst &= *src;
}

/* XOR */
static inline void memxor_64(u64 *dst, const u64 *src, void *arg)
{
        *dst ^= *src;
}

/* OR */
static inline void memor_64(u64 *dst, const u64 *src, void *arg)
{
        *dst |= *src;
}

static inline u8 *get_align64bit_ptr(void *addr)
{
        return (u8 *)ROUND_UP((u64)addr, ALIGN_64bit);
}

void dev_memand(void *restrict dst, const void *restrict src, int n)
{
        u8 *dev_dst, *dev_src;
        int w0, w1, w2;

        /* Short case */
        if (n < ALIGN_64bit) {
                dev_dst = (u8 *)dst;
                dev_src = (u8 *)src;
                template_memop64_short(dev_dst, dev_src, &=, n);
                return;
        }

        /* Long case */
        dev_dst = get_align64bit_ptr(dst);
        w0 = dev_dst - (u8 *)dst;
        w1 = n - w0;
        w2 = w1 % 8;
        w1 >>= 3;
        dev_src = (u8 *)src + w0;

        /* Head */
        template_memop64_head(dev_dst, dev_src, &=, w0);

        /* Middle */
        if (w1 > 1 && ((u64)dev_dst) % 16 != 0) {
                *(u64 *)dev_dst &= *(u64 *)dev_src;
                dev_dst += 8;
                dev_src += 8;
                --w1;
        }
        template_memop(u64, dev_dst, dev_src, w1, memand_64, NULL);

        /* Tail */
        dev_dst += (w1 << 3);
        dev_src += (w1 << 3);
        template_memop64_tail(dev_dst, dev_src, &=, w2);
}

void dev_memcpy(void *restrict dst, const void *restrict src, int n)
{
        u8 *dev_dst, *dev_src;
        int w0, w1, w2;

        /* Short case */
        if (n < ALIGN_64bit) {
                dev_dst = (u8 *)dst;
                dev_src = (u8 *)src;
                template_memop64_short(dev_dst, dev_src, =, n);
                return;
        }

        /* Long case */
        dev_dst = get_align64bit_ptr(dst);
        w0 = dev_dst - (u8 *)dst;
        w1 = n - w0;
        w2 = w1 % 8;
        w1 >>= 3;
        dev_src = (u8 *)src + w0;

        /* Head */
        template_memop64_head(dev_dst, dev_src, =, w0);

        /* Middle */
        if (w1 > 1 && (u64)dev_dst % 16 != 0) {
                *(u64 *)dev_dst = *(u64 *)dev_src;
                dev_dst += 8;
                dev_src += 8;
                --w1;
        }
        template_memop(u64, dev_dst, dev_src, w1, memcpy_64, NULL);

        /* Tail */
        dev_dst += (w1 << 3);
        dev_src += (w1 << 3);
        template_memop64_tail(dev_dst, dev_src, =, w2);
}

/* INVERT + AND */
void dev_memerase(void *restrict dst, const void *restrict src, int n)
{
        u8 *dev_dst, *dev_src;
        int w0, w1, w2;

        /* Short case */
        if (n < ALIGN_64bit) {
                dev_dst = (u8 *)dst;
                dev_src = (u8 *)src;
                template_memop64_short(dev_dst, dev_src, = ~, n);
                template_memop64_short(dev_dst, dev_src, &=, n);
                return;
        }

        /* Long case */
        dev_dst = get_align64bit_ptr(dst);
        w0 = dev_dst - (u8 *)dst;
        w1 = n - w0;
        w2 = w1 % 8;
        w1 >>= 3;
        dev_src = (u8 *)src + w0;

        /* Head */
        template_memop64_head(dev_dst, dev_src, = ~, w0);
        template_memop64_head(dev_dst, dev_src, &=, w0);

        /* Middle */
        if (w1 > 1 && (u64)dev_dst % 16 != 0) {
                *(u64 *)dev_dst = ~*(u64 *)dev_src;
                *(u64 *)dev_dst &= *(u64 *)dev_src;
                dev_dst += 8;
                dev_src += 8;
                --w1;
        }
        template_memop(u64, dev_dst, dev_src, w1, meminvertand_64, NULL);

        /* Tail */
        dev_dst += (w1 << 3);
        dev_src += (w1 << 3);
        template_memop64_tail(dev_dst, dev_src, = ~, w2);
        template_memop64_tail(dev_dst, dev_src, &=, w2);
}

void dev_memxor(void *restrict dst, const void *restrict src, int n)
{
        u8 *dev_dst, *dev_src;
        int w0, w1, w2;

        /* Short case */
        if (n < ALIGN_64bit) {
                dev_dst = (u8 *)dst;
                dev_src = (u8 *)src;
                template_memop64_short(dev_dst, dev_src, ^=, n);
                return;
        }

        /* Long case */
        dev_dst = get_align64bit_ptr(dst);
        w0 = dev_dst - (u8 *)dst;
        w1 = n - w0;
        w2 = w1 % 8;
        w1 >>= 3;
        dev_src = (u8 *)src + w0;

        /* Head */
        template_memop64_head(dev_dst, dev_src, ^=, w0);

        /* Middle */
        if (w1 > 1 && (u64)dev_dst % 16 != 0) {
                *(u64 *)dev_dst ^= *(u64 *)dev_src;
                dev_dst += 8;
                dev_src += 8;
                --w1;
        }
        template_memop(u64, dev_dst, dev_src, w1, memxor_64, NULL);

        /* Tail */
        dev_dst += (w1 << 3);
        dev_src += (w1 << 3);
        template_memop64_tail(dev_dst, dev_src, ^=, w2);
}

void dev_memor(void *restrict dst, const void *restrict src, int n)
{
        u8 *dev_dst, *dev_src;
        int w0, w1, w2;

        /* Short case */
        if (n < ALIGN_64bit) {
                dev_dst = (u8 *)dst;
                dev_src = (u8 *)src;
                template_memop64_short(dev_dst, dev_src, |=, n);
                return;
        }

        /* Long case */
        dev_dst = get_align64bit_ptr(dst);
        w0 = dev_dst - (u8 *)dst;
        w1 = n - w0;
        w2 = w1 % 8;
        w1 >>= 3;
        dev_src = (u8 *)src + w0;

        /* Head */
        template_memop64_head(dev_dst, dev_src, |=, w0);

        /* Middle */
        if (w1 > 1 && (u64)dev_dst % 16 != 0) {
                *(u64 *)dev_dst |= *(u64 *)dev_src;
                dev_dst += 8;
                dev_src += 8;
                --w1;
        }
        template_memop(u64, dev_dst, dev_src, w1, memor_64, NULL);

        /* Tail */
        dev_dst += (w1 << 3);
        dev_src += (w1 << 3);
        template_memop64_tail(dev_dst, dev_src, |=, w2);
}

void com_memand(void *restrict dst, const void *restrict src, int n)
{
        u8 *cdst = (u8 *)dst;
        u8 *csrc = (u8 *)src;
        int w0 = n >> 3;
        int w1 = n % 8;
        /* Head and middle */
        template_memop(u64, cdst, csrc, w0, memand_64, NULL);
        /* Tail */
        cdst += (w0 << 3);
        csrc += (w0 << 3);
        template_memop64_tail(cdst, csrc, &=, w1);
}

inline void com_memcpy(void *restrict dst, const void *restrict src, int n)
{
        memcpy(dst, src, n);
}

/* INVERT + AND */
void com_memerase(void *restrict dst, const void *restrict src, int n)
{
        u8 *cdst = (u8 *)dst;
        u8 *csrc = (u8 *)src;
        int w0 = n >> 3;
        int w1 = n % 8;
        /* Head and middle */
        template_memop(u64, cdst, csrc, w0, meminvertand_64, NULL);
        /* Tail */
        cdst += (w0 << 3);
        csrc += (w0 << 3);
        template_memop64_tail(cdst, csrc, = ~, w1);
        template_memop64_tail(cdst, csrc, &=, w1);
}

void com_memxor(void *restrict dst, const void *restrict src, int n)
{
        u8 *cdst = (u8 *)dst;
        u8 *csrc = (u8 *)src;
        int w0 = n >> 3;
        int w1 = n % 8;
        /* Head and middle */
        template_memop(u64, cdst, csrc, w0, memxor_64, NULL);
        /* Tail */
        cdst += (w0 << 3);
        csrc += (w0 << 3);
        template_memop64_tail(cdst, csrc, ^=, w1);
}

void com_memor(void *restrict dst, const void *restrict src, int n)
{
        u8 *cdst = (u8 *)dst;
        u8 *csrc = (u8 *)src;
        int w0 = n >> 3;
        int w1 = n % 8;
        /* Head and middle */
        template_memop(u64, cdst, csrc, w0, memor_64, NULL);
        /* Tail */
        cdst += (w0 << 3);
        csrc += (w0 << 3);
        template_memop64_tail(cdst, csrc, |=, w1);
}
