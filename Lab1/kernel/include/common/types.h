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

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <common/macro.h>
#include <uapi/types.h>

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef long long s64;
typedef int s32;
typedef short s16;
typedef signed char s8;

#define NULL ((void *)0)

typedef char bool;
#define true (1)
#define false (0)

#if __SIZEOF_POINTER__ == 4
typedef u32 paddr_t;
typedef u32 vaddr_t;
typedef u32 atomic_cnt;
#else
typedef u64 paddr_t;
typedef u64 vaddr_t;
typedef u64 atomic_cnt;
#endif

#ifdef CHCORE
#if __SIZEOF_POINTER__ == 4
typedef u32 clockid_t;
typedef u32 size_t;
#else
typedef u64 clockid_t;
typedef u64 size_t;
#endif

/** musl off_t is always 64-bit on any architectures */
typedef s64 off_t;
/** musl-1.2.0 enforces 64bit time_t on any architectures */
typedef u64 time_t;

/* The definition of timespec should be consistent with that in libc */
struct timespec {
	time_t tv_sec;
#ifdef BIG_ENDIAN
	int : 8 * (sizeof(time_t)-sizeof(long));
	long tv_nsec;
#else
	long tv_nsec;
	int : 8 * (sizeof(time_t)-sizeof(long));
#endif
};
#else
#include <sys/types.h>
#endif

#endif /* COMMON_TYPES_H */