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

#ifndef COMMON_MACRO_H
#define COMMON_MACRO_H

#ifdef CHCORE
#ifndef __ASM__
#include <common/kprint.h>
#include <common/backtrace.h>
#endif
#endif

#define ALIGN(n) __attribute__((__aligned__(n)))

#define ROUND_UP(x, n)		(((x) + (n) - 1) & ~((n) - 1))
#define ROUND_DOWN(x, n)	((x) & ~((n) - 1))
#define DIV_ROUND_UP(n, d)	(((n) + (d) - 1) / (d))

#define IS_ALIGNED(x, a)	(((x) & ((typeof(x))(a) - 1)) == 0)

#define BIT(x)			(1ULL << (x))

#define offsetof(TYPE, MEMBER)  ((size_t)&((TYPE *)0)->MEMBER)
#define container_of(ptr, type, field) \
	((type *)((void *)(ptr) - (void *)(&(((type *)(0))->field))))

#define container_of_safe(ptr, type, field) ({ \
	typeof (ptr) __ptr = (ptr); \
	type *__obj = container_of(__ptr, type, field); \
	(__ptr ? __obj : NULL); \
})

#define MAX(x, y)	((x) < (y) ? (y) : (x))
#define MIN(x, y)	((x) < (y) ? (x) : (y))

#define BUG_ON(expr) \
	do { \
		if ((unlikely(expr))) { \
			printk("BUG: %s:%d on (expr) %s\n", __func__, __LINE__, #expr); \
			backtrace(); \
			for (;;) { \
			} \
		} \
	} while (0)

#define BUG(str, ...) \
	do { \
		printk("BUG: %s:%d " str "\n", __func__, __LINE__, ##__VA_ARGS__); \
		backtrace(); \
		for (;;) { \
		} \
	} while (0)

#define WARN(msg) \
	printk("WARN: %s:%d %s\n", __func__, __LINE__, msg)

#define WARN_ON(cond, msg) \
	do { \
		if ((cond)) { \
			printk("WARN: %s:%d %s on " #cond "\n", \
			       __func__, __LINE__, msg); \
		} \
	} while (0)


#ifdef __GNUC__
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x)   (!!(x))
#define unlikely(x) (!!(x))
#endif  // __GNUC__

/*
 * Different platform may have different cacheline size
 * and may have some features like prefetch.
 */
#define CACHELINE_SZ 64
#define pad_to_cache_line(n) (ROUND_UP(n, CACHELINE_SZ) - (n))


/* Common enums */
#define OBJECT_STATE_INVALID (0)
#define OBJECT_STATE_VALID (1)

#endif /* COMMON_MACRO_H */