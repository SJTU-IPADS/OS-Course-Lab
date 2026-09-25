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

#include <common/types.h>

static inline u8 get8(u16 port)
{
	u8 data = 0;
	__asm__ __volatile__("inb %1, %0" : "=a"(data) : "d"(port));
	return data;
}

static inline void put8(u16 port, u8 data)
{
	__asm__ __volatile__("outb %0, %1" : : "a"(data), "d"(port));
}

static inline void put16(u16 port, u16 data)
{
	__asm__ __volatile__("outw %0, %1" : : "a"(data), "d"(port));
}

static inline u32 get32(u16 port)
{
	u32 data = 0;
	__asm__ __volatile__("inl %1, %0" : "=a"(data) : "d"(port));
	return data;
}

static inline void put32(u16 port, u32 data)
{
	__asm__ __volatile__("outl %0, %1" : : "a"(data), "d"(port));
}

static inline void pause(void)
{
	__asm__ __volatile__("pause"::);
}
