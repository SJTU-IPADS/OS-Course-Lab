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
	FIXME: this file is copied from seL4 which uses DATA61_GPL.
	We need to refer the manual and rewrite this file or
	refer to some other repo with MIT or FreeBSD license.
 */

#include <common/asm.h>
#include <arch/machine/registers.h>

#define TCR_T0SZ(x)       ((64 - (x)))
#define TCR_T1SZ(x)       ((64 - (x)) << 16)
#define TCR_TxSZ(x)       (TCR_T0SZ(x) | TCR_T1SZ(x))

#define TCR_IRGN0_WBWC    (1 << 8)
#define TCR_IRGN_NC       ((0 << 8) | (0 << 24))
#define TCR_IRGN_WBWA     ((1 << 8) | (1 << 24))
#define TCR_IRGN_WT       ((2 << 8) | (2 << 24))
#define TCR_IRGN_WBnWA    ((3 << 8) | (3 << 24))
#define TCR_IRGN_MASK     ((3 << 8) | (3 << 24))

#define TCR_ORGN0_WBWC    (1 << 10)
#define TCR_ORGN_NC       ((0 << 10) | (0 << 26))
#define TCR_ORGN_WBWA     ((1 << 10) | (1 << 26))
#define TCR_ORGN_WT       ((2 << 10) | (2 << 26))
#define TCR_ORGN_WBnWA    ((3 << 10) | (3 << 26))
#define TCR_ORGN_MASK     ((3 << 10) | (3 << 26))

#define TCR_SH0_ISH       (3 << 12)

#define TCR_TG0_4K        (0 << 14)
#define TCR_TG0_64K       (1 << 14)
#define TCR_TG1_4K        (2 << 30)
#define TCR_TG1_64K       (3 << 30)

#define TCR_PS_4G         (0 << 16)
#define TCR_PS_64G        (1 << 16)
#define TCR_PS_1T         (2 << 16)
#define TCR_PS_4T         (3 << 16)
#define TCR_PS_16T        (4 << 16)
#define TCR_PS_256T       (5 << 16)

/* bits are reserved as 1 */
#define TCR_EL2_RES1      ((1 << 23) | (1 << 31))
#define TCR_ASID16        (1 << 36)

#define UL(x) x##UL

#define TCR_SH0_SHIFT 12
#define TCR_SH0_MASK (UL(3) << TCR_SH0_SHIFT)
#define TCR_SH0_INNER (UL(3) << TCR_SH0_SHIFT)
#define TCR_SH1_SHIFT 28
#define TCR_SH1_MASK (UL(3) << TCR_SH1_SHIFT)
#define TCR_SH1_INNER (UL(3) << TCR_SH1_SHIFT)

#define TCR_SHARED (TCR_SH0_INNER | TCR_SH1_INNER)

#define TCR_TBI0 (UL(1) << 37)
#define TCR_A1   (UL(1) << 22)

#define MT_DEVICE_nGnRnE  0
#define MT_DEVICE_nGnRE   1
#define MT_DEVICE_GRE     2
#define MT_NORMAL_NC      3
#define MT_NORMAL         4
#define MAIR(_attr, _mt)  ((_attr) << ((_mt) * 8))

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

.macro dcache op
	dsb     sy
	mrs     x0, clidr_el1
	and     x3, x0, #0x7000000
	lsr     x3, x3, #23

	cbz     x3, finished_\op
	mov     x10, #0

	loop1_\op:
	add     x2, x10, x10, lsr #1
	lsr     x1, x0, x2
	and     x1, x1, #7
	cmp     x1, #2
	b.lt    skip_\op

	msr     csselr_el1, x10
	isb

	mrs     x1, ccsidr_el1
	and     x2, x1, #7
	add     x2, x2, #4
	mov     x4, #0x3ff
	and     x4, x4, x1, lsr #3
	clz     w5, w4
	mov     x7, #0x7fff
	and     x7, x7, x1, lsr #13

	loop2_\op:
	mov     x9, x4

	loop3_\op:
	lsl     x6, x9, x5
	orr     x11, x10, x6
	lsl     x6, x7, x2
	orr     x11, x11, x6
	dc      \op, x11
	subs    x9, x9, #1
	b.ge    loop3_\op
	subs    x7, x7, #1
	b.ge    loop2_\op

	skip_\op:
	add     x10, x10, #2
	cmp     x3, x10
	b.gt    loop1_\op

	finished_\op:
	mov     x10, #0
	msr     csselr_el1, x10
	dsb     sy
	isb
.endm

.macro enable_mmu sctlr tmp
	mrs     \tmp, \sctlr
	/* Enable MMU */
	orr     \tmp, \tmp, #SCTLR_EL1_M
	/* Disable alignment checking */
	bic     \tmp, \tmp, #SCTLR_EL1_A
	bic     \tmp, \tmp, #SCTLR_EL1_SA0
	bic     \tmp, \tmp, #SCTLR_EL1_SA
	orr     \tmp, \tmp, #SCTLR_EL1_nAA
	/* Data accesses Cacheable */
	orr     \tmp, \tmp, #SCTLR_EL1_C
	/* Instruction access Cacheable */
	orr     \tmp, \tmp, #SCTLR_EL1_I
	/* Writable eXecute Never */
	orr     x8, x8, #SCTLR_EL1_WXN
	msr     \sctlr, \tmp
	isb
.endm

.macro disable_mmu sctlr tmp
	mrs     \tmp, \sctlr
	/* Disable MMU */
	bic     \tmp, \tmp, #SCTLR_EL1_M
	/* Disable alignment checking */
	bic     \tmp, \tmp, #SCTLR_EL1_A
	bic     \tmp, \tmp, #SCTLR_EL1_SA0
	bic     \tmp, \tmp, #SCTLR_EL1_SA
	orr     \tmp, \tmp, #SCTLR_EL1_nAA
	/* Disable Data Cache */
	bic     \tmp, \tmp, #SCTLR_EL1_C
	/* Disable Instruction Cache */
	bic     \tmp, \tmp, #SCTLR_EL1_I
	msr     \sctlr, \tmp
	isb
.endm

BEGIN_FUNC(arm64_elX_to_el1)
	mrs     x9, CurrentEL

	// Check the current exception level.
	cmp     x9, CURRENTEL_EL1
	beq     .Ltarget
	cmp     x9, CURRENTEL_EL2
	beq     .Lin_el2
	// Otherwise, we are in EL3.

	// Set EL2 to 64bit and enable the HVC instruction.
	mrs     x9, scr_el3
	mov     x10, SCR_EL3_NS | SCR_EL3_HCE | SCR_EL3_RW
	orr     x9, x9, x10
	msr     scr_el3, x9

	// Set the return address and exception level.
	adr     x9, .Ltarget
	msr     elr_el3, x9
	mov     x9, SPSR_ELX_DAIF | SPSR_ELX_EL1H
	msr     spsr_el3, x9

	.Lin_el2:
	// Disable EL1 timer traps and the timer offset.
	mrs     x9, cnthctl_el2
	orr     x9, x9, CNTHCTL_EL2_EL1PCEN | CNTHCTL_EL2_EL1PCTEN
	msr     cnthctl_el2, x9
	msr     cntvoff_el2, xzr

	// Disable EL2 coprocessor traps.
	mov     x9, CPTR_EL2_RES1
	msr     cptr_el2, x9

	msr     hstr_el2, xzr
	// Disable stage 2 translations.
	msr     vttbr_el2, xzr

	// Disable EL1 FPU traps.
	mov     x9, CPACR_EL1_FPEN
	msr     cpacr_el1, x9

	// Check whether the GIC system registers are supported.
	mrs     x9, id_aa64pfr0_el1
	and     x9, x9, ID_AA64PFR0_EL1_GIC
	cbz     x9, .Lno_gic_sr

	// Enable the GIC system registers in EL2, and allow their use in EL1.
	mrs     x9, ICC_SRE_EL2
	mov     x10, ICC_SRE_EL2_ENABLE | ICC_SRE_EL2_SRE
	orr     x9, x9, x10
	msr     ICC_SRE_EL2, x9

	// Disable the GIC virtual CPU interface.
	msr     ICH_HCR_EL2, xzr

	disable_mmu sctlr_el1, x9

	.Lno_gic_sr:
	// Set EL1 to 64bit.
	mov     x9, HCR_EL2_RW
	msr     hcr_el2, x9

	// Set the return address and exception level.
	adr     x9, .Ltarget
	msr     elr_el2, x9
	mov     x9, #(SPSR_ELX_DAIF | SPSR_ELX_EL1H)
	msr     spsr_el2, x9

	isb
	eret

	.Ltarget:
	ret
END_FUNC(arm64_elX_to_el1)

BEGIN_FUNC(invalidate_dcache)
	dcache  isw
	ret
END_FUNC(invalidate_dcache)

BEGIN_FUNC(invalidate_icache)
	ic      iallu
	dsb     nsh
	isb
	ret
END_FUNC(invalidate_icache)

.extern boot_ttbr0_l0
.extern boot_ttbr1_l0

BEGIN_FUNC(el1_mmu_activate)
	/* We call nested functions, follow the ABI. */
	stp     x29, x30, [sp, #-16]!
	mov     x29, sp

	bl	invalidate_dcache 

	/* Ensure I-cache, D-cache and mmu are disabled for EL1/Stage1 */
	disable_mmu sctlr_el1 , x8

	/*
	 * Invalidate the local I-cache so that any instructions fetched
	 * speculatively are discarded.
	 */
	bl	invalidate_icache

	/*
	 *   DEVICE_nGnRnE      000     00000000
	 *   DEVICE_nGnRE       001     00000100
	 *   DEVICE_GRE         010     00001100
	 *   NORMAL_NC          011     01000100
	 *   NORMAL             100     11111111
	 */
	ldr     x5, =MAIR(0x00, MT_DEVICE_nGnRnE) |\
		     MAIR(0x04, MT_DEVICE_nGnRE) |\
		     MAIR(0x0c, MT_DEVICE_GRE) |\
		     MAIR(0x44, MT_NORMAL_NC) |\
		     MAIR(0xff, MT_NORMAL)

	msr     mair_el1, x5

	ldr     x10, =TCR_TxSZ(48) | TCR_IRGN_WBWA | TCR_ORGN_WBWA |\
		      TCR_TG0_4K | TCR_TG1_4K | TCR_ASID16 | TCR_SHARED

	mrs     x9, ID_AA64MMFR0_EL1
	bfi     x10, x9, #32, #3
	msr     tcr_el1, x10


	/* Setup page tables */
	adrp    x8, boot_ttbr0_l0
	msr     ttbr0_el1, x8
	adrp    x8, boot_ttbr1_l0 
	msr     ttbr1_el1, x8
	isb

	/* invalidate all TLB entries for EL1 */
	tlbi    vmalle1is
	dsb     ish
	isb

	enable_mmu sctlr_el1 , x8

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
