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

#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#include <common/types.h>

#ifdef CHCORE
/*
 * memcpy does not handle: dst and src overlap.
 * memmove does.
 */
void memcpy(void *dst, const void *src, size_t size);
void memmove(void *dst, const void *src, size_t size);

#if defined(CHCORE_ARCH_AARCH64) || defined(CHCORE_ARCH_SPARC)
int __copy_to_user(void *dst, const void *src, size_t size);
int __copy_from_user(void *dst, const void *src, size_t size);
#endif

/* use bzero instead of memset(buf, 0, size) */
void bzero(void *p, size_t size);

void memset(void *dst, char ch, size_t size);

static inline int strcmp(const char *src, const char *dst)
{
	while (*src && *dst) {
		if (*src == *dst) {
			src++;
			dst++;
			continue;
		}
		return *src - *dst;
	}
	if (!*src && !*dst)
		return 0;
	if (!*src)
		return -1;
	return 1;
}

static inline int strncmp(const char *src, const char *dst, size_t size)
{
	size_t i;

	for (i = 0; i < size; ++i) {
		if (src[i] == '\0' || src[i] != dst[i])
			return src[i] - dst[i];
	}

	return 0;
}

static inline size_t strlen(const char *src)
{
	size_t i = 0;

	while (*src++)
		i++;

	return i;
}

/*
 * Compares the first @n bytes (each interpreted as unsigned char)
 * of the memory areas @s1 and @s2.
*/
static inline int memcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *l = s1, *r = s2;
	for (; n && *l == *r; n--, l++, r++);
	return n ? *l - *r : 0;
}
#endif

#endif /* COMMON_UTIL_H */
