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

#ifndef COMMON_BITOPS_H
#define COMMON_BITOPS_H


#define BITS_PER_BYTE		8
#define BITS_PER_INT		(sizeof(unsigned int) * BITS_PER_BYTE)
#define BITS_PER_LONG		(sizeof(unsigned long) * BITS_PER_BYTE)
#define BITS_TO_LONGS(nr)	DIV_ROUND_UP(nr, BITS_PER_LONG)
#define GENMASK_ULL(h, l) \
	(((~0ULL) - (1ULL << (l)) + 1) & \
	 (~0ULL >> (64 - 1 - (h))))

#define get_bit_in_slot(u64slot, bit) ((u64slot) & (1UL<<(bit)))

#define clear_bit_in_slot(u64slot, bit) ((u64slot) = (u64slot) & ~(1UL<<(bit)))

#define set_bit_in_slot(u64slot, bit) ((u64slot) = (u64slot) | (1UL<<(bit)))

/* set No'nr bit to 1 in slot pointed by p */
static inline void set_bit(unsigned int nr, volatile unsigned long *p)
{
	unsigned nlongs = nr / BITS_PER_LONG;
	unsigned ilongs = nr % BITS_PER_LONG;
	p[nlongs] |= 1UL << ilongs;
}

/* get No'nr bit in slot pointed by p */
static inline int get_bit(unsigned int nr, volatile unsigned long *p)
{
	unsigned nlongs = nr / BITS_PER_LONG;
	unsigned ilongs = nr % BITS_PER_LONG;
	return (p[nlongs] >> ilongs) & 0x1;
}

/* clear No'nr bit in slot pointed by p */
static inline void clear_bit(unsigned int nr, volatile unsigned long *p)
{
	unsigned nlongs = nr / BITS_PER_LONG;
	unsigned ilongs = nr % BITS_PER_LONG;
	p[nlongs] &= ~(1UL << ilongs);
}

/* return the first one bit start from the lowest bit */
static inline int ctzl(unsigned long x)
{
	return x == 0 ? BITS_PER_LONG : __builtin_ctzl(x);
}

/* return the first one bit start from the highest bit */
static inline int bsr(unsigned int x)
{
#ifndef CHCORE_ARCH_RISCV64
	return BITS_PER_INT - 1 - __builtin_clz(x);
#else
	/* FIXME: ineffient but simple implementation */
	int i;
	for (i = BITS_PER_INT - 1; !(x & BIT(i)); i--);
	return i;
#endif
}

static int find_next_bit_helper(unsigned long *p, unsigned long size,
				     unsigned long start, int invert)
{
	long cur_elem_index, cur_bit,
		 max_elem_index, max_bit,
		 cur_bit_value, res = 0;

	max_elem_index = (size - 1) / BITS_PER_LONG;
	cur_elem_index = start / BITS_PER_LONG;
	cur_bit = start % BITS_PER_LONG;
	res = start;

	while (cur_elem_index <= max_elem_index) {
		if (cur_elem_index < max_elem_index)
			max_bit = BITS_PER_LONG - 1;
		else
			max_bit = (size - 1) % BITS_PER_LONG;
		for (; cur_bit <= max_bit; cur_bit++, res++) {
			cur_bit_value = get_bit_in_slot(p[cur_elem_index], cur_bit);
			if (invert ? !cur_bit_value : cur_bit_value)
				return res;
		}
		cur_elem_index++;
		cur_bit = 0;
	}
	return size;
}

/* 
 * From lowest bit side, starting from 'start', 
 * this function find the first zero bit of the slot pointed by p. 
 */
static inline int find_next_zero_bit(unsigned long *p, unsigned long size,
				     unsigned long start)
{
	return find_next_bit_helper(p, size, start, 1);
}

/* 
 * From lowest bit side, starting from 'start', 
 * this function find the first bit of the slot pointed by p. 
 */
static inline int find_next_bit(unsigned long *p, unsigned long size,
				unsigned long start)
{
	return find_next_bit_helper(p, size, start, 0);
}

/* From the first 1 bit to the last 1 bit in slot pointed by addr */
#define for_each_set_bit(pos, addr, size)		\
	for ((pos) = find_next_bit((addr), (size), 0);	\
	     (pos) < (size);				\
             (pos) = find_next_bit((addr), (size), (pos) + 1))


#endif /* COMMON_BITOPS_H */