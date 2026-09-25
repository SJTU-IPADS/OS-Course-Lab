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

// Copyright 2016 The Fuchsia Authors
// Copyright (c) 2014, Google Inc. All rights reserved
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

/*
   MIT License

   Copyright (c) 2018 Sergey Matyukevich

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#include <common/asm.h>
#include <arch/machine/registers.h>

#define CURRENTEL_EL1           (0b01 << 2)
#define CURRENTEL_EL2           (0b10 << 2)

#define CPACR_EL1_FPEN          (0b11 << 20)
#define ID_AA64PFR0_EL1_GIC     (0b1111 << 24)

#define CNTHCTL_EL2_EL1PCEN     (1 << 1)
#define CNTHCTL_EL2_EL1PCTEN    (1 << 0)
#define CPTR_EL2_RES1           0x33ff
#define HCR_EL2_RW              (1 << 31)
#define ICC_SRE_EL2_SRE         (1 << 0)
#define ICC_SRE_EL2_ENABLE      (1 << 3)

#define SCR_EL3_HCE             (1 << 8)
#define SCR_EL3_NS              (1 << 0)
#define SCR_EL3_RW              (1 << 10)

#define SPSR_ELX_DAIF           (0b1111 << 6)
#define SPSR_ELX_EL1H           (0b0101)

#define ICH_HCR_EL2             S3_4_C12_C11_0
#define ICC_SRE_EL2             S3_4_C12_C9_5

BEGIN_FUNC(arm64_elX_to_el1)
	mrs x9, CurrentEL

	// Check the current exception level.
	cmp x9, CURRENTEL_EL1
	beq .Ltarget
	cmp x9, CURRENTEL_EL2
	beq .Lin_el2
	// Otherwise, we are in EL3.

	// Set EL2 to 64bit and enable the HVC instruction.
	mrs x9, scr_el3
	mov x10, SCR_EL3_NS | SCR_EL3_HCE | SCR_EL3_RW
	orr x9, x9, x10
	msr scr_el3, x9

	// Set the return address and exception level.
	adr x9, .Ltarget
	msr elr_el3, x9
	mov x9, SPSR_ELX_DAIF | SPSR_ELX_EL1H
	msr spsr_el3, x9

.Lin_el2:
	// Disable EL1 timer traps and the timer offset.
	mrs x9, cnthctl_el2
	orr x9, x9, CNTHCTL_EL2_EL1PCEN | CNTHCTL_EL2_EL1PCTEN
	msr cnthctl_el2, x9
	msr cntvoff_el2, xzr

	// Disable stage 2 translations.
	msr vttbr_el2, xzr

	// Disable EL2 coprocessor traps.
	mov x9, CPTR_EL2_RES1
	msr cptr_el2, x9

	// Disable EL1 FPU traps.
	mov x9, CPACR_EL1_FPEN
	msr cpacr_el1, x9

	// Check whether the GIC system registers are supported.
	mrs x9, id_aa64pfr0_el1
	and x9, x9, ID_AA64PFR0_EL1_GIC
	cbz x9, .Lno_gic_sr

	// Enable the GIC system registers in EL2, and allow their use in EL1.
	mrs x9, ICC_SRE_EL2
	mov x10, ICC_SRE_EL2_ENABLE | ICC_SRE_EL2_SRE
	orr x9, x9, x10
	msr ICC_SRE_EL2, x9

	// Disable the GIC virtual CPU interface.
	msr ICH_HCR_EL2, xzr

.Lno_gic_sr:
	// Set EL1 to 64bit.
	mov x9, HCR_EL2_RW
	msr hcr_el2, x9

	// Set the return address and exception level.
	adr x9, .Ltarget
	msr elr_el2, x9
	mov x9, SPSR_ELX_DAIF | SPSR_ELX_EL1H
	msr spsr_el2, x9

	isb
	eret

.Ltarget:
	ret
END_FUNC(arm64_elX_to_el1)

// See https://developer.arm.com/documentation/den0024/a/Caches/Cache-maintenance
BEGIN_FUNC(invalidate_cache_all)
	mrs     x0, clidr_el1
	and     w3, w0, #0x07000000     // get 2x level of coherence
	lsr     w3, w3, #23
	cbz     w3, .Lfinished_inv_cache
	mov     w10, #0                 // w10 = 2x cache level
	mov     w8, #1                  // w8 = constant 1
.Lloop1_inv_cache:
	add     w2, w10, w10, lsr #1    // calculate 3x cache level
	lsr     w1, w0, w2              // extract 3 bit cache type for this level
	and     w1, w1, #0x7
	cmp     w1, #2
	b.lt    .Lskip_inv_cache        // no data or unified cache at this level
	msr     csselr_el1, x10         // select this cache level
	isb                             // synchronize change to csselr
	mrs     x1, ccsidr_el1          // w1 = ccsidr
	and     w2, w1, #7              // w2 = log2(line len) - 4
	add     w2, w2, #4              // w2 = log2(line len)
	ubfx    w4, w1, #3, #10         // w4 = max way number, right aligned
	clz     w5, w4                  // w5 = 32 - log2(ways), bit position of way in DC operand
	lsl     w9, w4, w5              // w9 = max way number, aligned to position in DC operand
	lsl     w12, w8, w5             // w12 = amount to decrement way number per iteration

.Lloop2_inv_cache:
	ubfx    w7, w1, #13, #15        // w7 = max set number, right aligned
	lsl     w7, w7, w2              // w7 = max set number, aligned to position in DC operand
	lsl     w13, w8, w2             // w13 = amount to decrement set number per iteration
.Lloop3_inv_cache:
	orr     w11, w10, w9            // w11 = combine way number and cache number
	orr     w11, w11, w7            //       and set number for DC operand
	dc      isw, x11                // data cache op
	subs    w7, w7, w13             // decrement set number
	b.ge    .Lloop3_inv_cache

	subs    x9, x9, x12             // decrement way number
	b.ge    .Lloop2_inv_cache
.Lskip_inv_cache:
	add     w10, w10, #2            // increment 2x cache level
	cmp     w3, w10
	dsb     sy                      // ensure completetion of previous cache maintainance instructions
	b.gt    .Lloop1_inv_cache
.Lfinished_inv_cache:

	// dump the instruction cache as well
	ic      iallu
	isb
	ret
END_FUNC(invalidate_cache_all)

.extern boot_ttbr0_l0
.extern boot_ttbr1_l0


/* DEVICE_nGnRnE */
#define MMU_MAIR_ATTR0		(0x00 << (8 * 0))

/* DEVICE_nGnRE */
#define MMU_MAIR_ATTR1		(0x04 << (8 * 1))

/* DEVICE_GRE */
#define MMU_MAIR_ATTR2		(0x0c << (8 * 2))

/* NORMAL_NC */
#define MMU_MAIR_ATTR3          (0x44 << (8 * 3))

/* NORMAL */
#define MMU_MAIR_ATTR4          (0xff << (8 * 4))

/*
 * Enable cached page table walks:
 * inner/outer (IRGN/ORGN): write-back + write-allocate
 */
#define MMU_TCR_TG1_4k 			(0 << 14)
#define MMU_TCR_SH1_INNER_SH	        (3 << 28)
#define MMU_TCR_ORGN1_WBA		(1 << 26)
#define MMU_TCR_IRGN1_WBA		(1 << 24)
#define MMU_TCR_T1SZ			((64 - 48) << 16) /* 48-bit  */
#define MMU_TCR_FLAGS1			(MMU_TCR_TG1_4k | MMU_TCR_SH1_INNER_SH | \
 						MMU_TCR_ORGN1_WBA | MMU_TCR_IRGN1_WBA | MMU_TCR_T1SZ)

#define MMU_TCR_TG0_4k 			(0 << 30)
#define MMU_TCR_SH0_INNER_SH	        (3 << 12)
#define MMU_TCR_ORGN0_WBA		(1 << 10)
#define MMU_TCR_IRGN0_WBA		(1 << 8)
#define MMU_TCR_T0SZ			((64 - 48) << 0) /* 48-bit */
#define MMU_TCR_FLAGS0			(MMU_TCR_TG0_4k | MMU_TCR_SH0_INNER_SH | \
 						MMU_TCR_ORGN0_WBA | MMU_TCR_IRGN0_WBA | MMU_TCR_T0SZ)
#define MMU_TCR_IPS 			(0b101 << 32) /* 48-bit */
#define MMU_TCR_AS				(1 << 36)


BEGIN_FUNC(el1_mmu_activate)
	stp     x29, x30, [sp, #-16]!
	mov     x29, sp

	bl	invalidate_cache_all

	/* Invalidate TLB */
	tlbi    vmalle1is
	isb
	dsb     sy

	/* Initialize Memory Attribute Indirection Register */
	ldr 	x8, =MMU_MAIR_ATTR0 | MMU_MAIR_ATTR1 | MMU_MAIR_ATTR2 | MMU_MAIR_ATTR3 | MMU_MAIR_ATTR4
	msr     mair_el1, x8

	/* Initialize TCR_EL1 */
	/* set cacheable attributes on translation walk */
	/* (SMP extensions) non-shareable, inner write-back write-allocate */
	ldr  	x8, =MMU_TCR_FLAGS1 | MMU_TCR_FLAGS0 | MMU_TCR_IPS | MMU_TCR_AS
	msr     tcr_el1, x8
	isb

	/* Write ttbr with phys addr of the translation table */
	adrp    x8, boot_ttbr0_l0
	msr     ttbr0_el1, x8
	adrp    x8, boot_ttbr1_l0
	msr     ttbr1_el1, x8
	isb

	mrs     x8, sctlr_el1
	/* Enable MMU */
	orr     x8, x8, #SCTLR_EL1_M
	/* Disable alignment checking */
	bic     x8, x8, #SCTLR_EL1_A
	bic     x8, x8, #SCTLR_EL1_SA0
	bic     x8, x8, #SCTLR_EL1_SA
	orr     x8, x8, #SCTLR_EL1_nAA
	/* Data accesses Cacheable */
	orr     x8, x8, #SCTLR_EL1_C
	/* Instruction access Cacheable */
	orr     x8, x8, #SCTLR_EL1_I
	/* Writable eXecute Never */
	orr     x8, x8, #SCTLR_EL1_WXN
	msr     sctlr_el1, x8

	ldp     x29, x30, [sp], #16
	ret
END_FUNC(el1_mmu_activate)


/* These simple function is written by ChCorers */
BEGIN_FUNC(early_put32)
	str w1, [x0]
	ret
END_FUNC(early_put32)

BEGIN_FUNC(early_get32)
	ldr w0, [x0]
	ret
END_FUNC(early_get32)

BEGIN_FUNC(delay)
	subs x0, x0, #1
	bne delay
	ret
END_FUNC(delay)
