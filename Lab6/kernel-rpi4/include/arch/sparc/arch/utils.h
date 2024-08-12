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

inline unsigned get32(unsigned addr)
{
	unsigned ret;
	asm volatile("ld [%1], %0\n"
		:"=r"(ret)
		:"r"(addr));
	return ret;
}

inline void put32(unsigned addr, unsigned val)
{
	asm volatile("st %0, [%1]\n"
		: :"r"(val), "r"(addr)
		:"memory");
}

inline unsigned get32_asi(unsigned addr, unsigned asi)
{
	unsigned ret;
	asm volatile("lda [%1] %2, %0\n"
		:"=r"(ret)
		:"r"(addr), "i"(asi));
	return ret;
}

inline void put32_asi(unsigned addr, unsigned asi, unsigned val)
{
	asm volatile("sta %0, [%1] %2\n"
		: :"r"(val), "r"(addr), "i"(asi)
		:"memory");
}