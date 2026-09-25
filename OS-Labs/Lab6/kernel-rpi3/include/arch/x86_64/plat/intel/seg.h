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

#include <machine.h>

/* max TSS number */
#define TSS_ENTRIES     (PLAT_CPU_NUM * 2)

#define GDT_NULL         0
#define GDT_CS_32        1
#define GDT_CS_64        2
#define GDT_DS           3
#define GDT_FS           4
#define GDT_GS           5
#define GDT_UD           6
#define GDT_UC           7
/* each TSS seg occupies two entries */
#define GDT_TSS_BASE_LO  8
#define GDT_TSS_BASE_HI  9
#define GDT_ENTRIES      (8 + TSS_ENTRIES + 1)

#define KCSEG32         (GDT_CS_32<<3)	/* kernel 32-bit code segment */
#define KCSEG64         (GDT_CS_64<<3)	/* kernel code segment */
#define KDSEG           (GDT_DS<<3)	/* kernel data segment */
#define KFSSEG          (GDT_FS<<3)
#define KGSSEG          (GDT_GS<<3)
#define UDSEG           (GDT_UD<<3)	/* user data segment */
#define UCSEG           (GDT_UC<<3)	/* user code segment */


/* From sv6, Copyright (c) MIT LICENSE */
// User segment bits (SEG_S set).
#define SEG_A           (1<<0)	/* segment accessed bit */
#define SEG_R           (1<<1)	/* readable (code) */
#define SEG_W           (1<<1)	/* writable (data) */
#define SEG_C           (1<<2)	/* conforming segment (code) */
#define SEG_E           (1<<2)	/* expand-down bit (data) */
#define SEG_CODE        (1<<3)	/* code segment (instead of data) */

// System segment bits (SEG_S is clear).
#define SEG_LDT         (2<<0)	/* local descriptor table */
#define SEG_TSS64A      (9<<0)	/* available 64-bit TSS */
#define SEG_TSS64B      (11<<0)	/* busy 64-bit TSS */
#define SEG_CALL64      (12<<0)	/* 64-bit call gate */
#define SEG_INTR64      (14<<0)	/* 64-bit interrupt gate */
#define SEG_TRAP64      (15<<0)	/* 64-bit trap gate */

// User and system segment bits.
#define SEG_S           (1<<4)	/* if 0, system descriptor */
#define SEG_DPL(x)      ((x)<<5)	/* descriptor privilege level (2 bits) */
#define SEG_P           (1<<7)	/* segment present */
#define SEG_AVL         (1<<8)	/* available for operating system use */
#define SEG_L           (1<<9)	/* long mode */
#define SEG_D           (1<<10)	/* default operation size 32-bit */
#define SEG_G           (1<<11)	/* granularity */


// SEGDESC constructs a segment descriptor literal
// with the given, base, limit, and type bits.
#ifdef __ASM__
#define SEGDESC(base, limit, bits) \
	.word	(limit)&0xffff, (base)&0xffff; \
	.byte	((base)>>16)&0xff, \
		(bits)&0xff, \
		(((bits)>>4)&0xf0) | (((limit)>>16)&0xf), \
		((base)>>24)&0xff;
#else
#define SEGDESC(base, limit, bits) { \
	(limit)&0xffff, (u16) ((base)&0xffff), \
	(u8) (((base)>>16)&0xff), \
	(bits)&0xff, \
	(((bits)>>4)&0xf0) | (((limit)>>16)&0xf), \
	(u8) (((base)>>24)&0xff), \
}
#endif

// SEGDESCHI constructs an extension segment descriptor
// literal that records the high bits of base.
#define SEGDESCHI(base) { \
	(u16) (((base)>>32)&0xffff), (u16) (((base)>>48)&0xffff), \
}

#ifndef __ASM__
#include <common/types.h>
struct segdesc {
	u16 limit0;
	u16 base0;
	u8 base1;
	u8 bits;
	u8 bitslimit1;
	u8 base2;
};

struct desc_ptr {
	u16 size;
	u64 address;
} __attribute__((packed, aligned(16))) ;

extern struct segdesc bootgdt[GDT_ENTRIES];
#endif
