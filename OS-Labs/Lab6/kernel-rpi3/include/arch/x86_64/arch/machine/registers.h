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

#include <common/macro.h>

/* EFLAGS Value */
#define EFLAGS_CF               BIT(0)
#define EFLAGS_PF               BIT(2)
#define EFLAGS_AF               BIT(4)
#define EFLAGS_ZF               BIT(6)
#define EFLAGS_SF               BIT(7)
#define EFLAGS_TF               BIT(8)	/* Trap */
#define EFLAGS_IF               BIT(9)	/* Interrupt enable */
#define EFLAGS_DF               BIT(10)
#define EFLAGS_OF               BIT(11)
#define EFLAGS_NT               BIT(14)	/* Nested task */
#define EFLAGS_RF               BIT(16)	/* Resume */
#define EFLAGS_VM               BIT(17)	/* Virtual 8086 */
#define EFLAGS_AC               BIT(18)	/* Alignment check */
#define EFLAGS_VIF              BIT(19)	/* Virtual interrupt */
#define EFLAGS_VIP              BIT(20)	/* Virtual interrupt padding */
#define EFLAGS_ID               BIT(21)	/* Identification */
#define EFLAGS_1                BIT(1)	/* Should be 1 */
#define EFLAGS_0                (BIT(3) | BIT(5))	/* Should be 0 */

#define EFLAGS_DEFAULT          EFLAGS_IF | EFLAGS_1

/*
 * See eret_to_thread:
 * reuse EC to distinguish how to return to user-level
 */
#define EC_SYSEXIT              (-1)

#ifndef __ASM__
enum reg_type {
	RESERVE = 0,		/* 0x00 Reserved to pad */
	DS = 1,			/* 0x08 */

	/* Callee saved */
	R15 = 2,		/* 0x10 */
	R14 = 3,		/* 0x18 */
	R13 = 4,		/* 0x20 */
	R12 = 5,		/* 0x28 */
	RBP = 6,		/* 0x30 */
	RBX = 7,		/* 0x38 */

	/* Caller saved */
	R11 = 8,		/* 0x40 */
	R10 = 9,		/* 0x48 */
	R9 = 10,		/* 0x50 */
	R8 = 11,		/* 0x58 */
	RAX = 12,		/* 0x60 */
	RCX = 13,		/* 0x68 */
	RDX = 14,		/* 0x70 */
	RSI = 15,		/* 0x78 */
	RDI = 16,		/* 0x80 */

	TRAPNO = 17,		/* 0x88 Trap number saved by us */

	/*
	 * When a trap happens, hardware saves the following states.
	 * For traps without EC, our trap handler will push a zero.
	 */
	EC = 18,		/* 0x90 Error Code */
	RIP = 19,		/* 0x98 */
	CS = 20,		/* 0xa0 */
	RFLAGS = 21,		/* 0xa8 */
	RSP = 22,		/* 0xb0 */
	SS = 23,		/* 0xb8 */
};

/* ptrace registers */
struct chcore_user_regs_struct {
	unsigned long dummy_reserved, ds, r15, r14, r13, r12, rbp, rbx;
	unsigned long r11, r10, r9, r8, rax, rcx, rdx, rsi, rdi, trapno;
	unsigned long dummy_ec, rip, cs, eflags, rsp, ss;
	/* not in arch_exec_ctx_t regs */
	unsigned long fs_base, gs_base, es, fs, gs;
};
#endif /* ASMCODE */

#define REG_NUM         24

/* offset in tls_base_reg */
#define TLS_FS		0
#define TLS_GS		1

#define SZ_U64			8
#define ARCH_EXEC_CONT_SIZE 	REG_NUM*SZ_U64	/* Cannot use sizeof in asm */
