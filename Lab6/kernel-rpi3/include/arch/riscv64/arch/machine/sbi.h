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
 * Copyright (c) 2016-2017 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 * Copyright (c) 2019 Mitchell Horne <mhorne@FreeBSD.org>
 *
 * Portions of this software were developed by SRI International and the
 * University of Cambridge Computer Laboratory under DARPA/AFRL contract
 * FA8750-10-C-0237 ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Portions of this software were developed by the University of Cambridge
 * Computer Laboratory as part of the CTSRD Project, with support from the
 * UK Higher Education Innovation Fund (HEIF).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

/* Reference: FreeBSD sbi.h sbi.c */

#pragma once

#include <common/types.h>

#define SBI_SET_TIMER              0
#define SBI_CONSOLE_PUTCHAR        1
#define SBI_CONSOLE_GETCHAR        2
#define SBI_CLEAR_IPI              3
#define SBI_SEND_IPI               4
#define SBI_REMOTE_FENCE_I         5
#define SBI_REMOTE_SFENCE_VMA      6
#define SBI_REMOTE_SFENCE_VMA_ASID 7
#define SBI_SHUTDOWN               8
#define SBI_HSM                    0x48534D

struct sbi_ret {
	long error;
	long value;
};
static inline struct sbi_ret sbi_call(u64 arg7, u64 arg6, u64 arg0, u64 arg1,
				      u64 arg2, u64 arg3, u64 arg4)
{
	struct sbi_ret ret;

	register u64 a0 asm("a0") = (arg0);
	register u64 a1 asm("a1") = (arg1);
	register u64 a2 asm("a2") = (arg2);
	register u64 a3 asm("a3") = (arg3);
	register u64 a4 asm("a4") = (arg4);
	register u64 a6 asm("a6") = (arg6);
	register u64 a7 asm("a7") = (arg7);

	asm volatile("ecall"
		     : "+r"(a0), "+r"(a1)
		     : "r"(a2), "r"(a3), "r"(a4), "r"(a6), "r"(a7)
		     : "memory");
	ret.error = a0;
	ret.value = a1;
	return (ret);
}

static inline void sbi_console_putchar(char ch)
{
	sbi_call(SBI_CONSOLE_PUTCHAR, 0, (unsigned long)ch, 0, 0, 0, 0);
}

static inline int sbi_console_getchar(void)
{
	return (int)sbi_call(SBI_CONSOLE_GETCHAR, 0, 0, 0, 0, 0, 0).error;
}

static inline void sbi_set_timer(u64 stime_value)
{
#if __riscv_xlen == 32
	SBI_CALL(SBI_SET_TIMER, 0, stime_value, stime_value >> 32, 0, 0, 0, 0);
#else
	sbi_call(SBI_SET_TIMER, 0, stime_value, 0, 0, 0, 0);
#endif
}

static inline void sbi_shutdown(void)
{
	sbi_call(SBI_SHUTDOWN, 0, 0, 0, 0, 0, 0);
}

static inline void sbi_clear_ipi(void)
{
	sbi_call(SBI_CLEAR_IPI, 0, 0, 0, 0, 0, 0);
}

static inline void sbi_send_ipi(const u64 *hart_mask)
{
	sbi_call(SBI_SEND_IPI, 0, (u64)hart_mask, 0, 0, 0, 0);
}

static inline void sbi_remote_fence_i(const u64 *hart_mask)
{
	sbi_call(SBI_REMOTE_FENCE_I, 0, (u64)hart_mask, 0, 0, 0, 0);
}

static inline void sbi_remote_sfence_vma(const u64 *hart_mask,
					 u64 start,
					 u64 size)
{
	sbi_call(SBI_REMOTE_SFENCE_VMA, 0, (u64)hart_mask, start, size, 0, 0);
}

static inline void sbi_remote_sfence_vma_asid(const u64 *hart_mask,
					      u64 start,
					      u64 size,
					      u64 asid)
{
	sbi_call(SBI_REMOTE_SFENCE_VMA_ASID,
		 0,
		 (u64)hart_mask,
		 start,
		 size,
		 asid,
		 0);
}

static inline void sbi_debug(void)
{
	sbi_call(9, 0, 0, 0, 0, 0, 0);
}
static inline void sbi_hart_start(u64 hartid,
				  u64 start_addr)
{
	sbi_call(SBI_HSM, 0, hartid, start_addr, 0, 0, 0);
}
