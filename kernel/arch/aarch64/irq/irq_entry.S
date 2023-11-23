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

#include <common/asm.h>
#include <common/debug.h>
#include <common/vars.h>
#include <arch/machine/registers.h>
#include <arch/machine/esr.h>
#include <arch/machine/smp.h>

#include "irq_entry.h"

.extern syscall_table
.extern hook_syscall
.extern finish_switch
.extern do_pending_resched

.macro	exception_entry	label
	/* Each entry of the exeception table should be 0x80 aligned */
	.align	7
	b	\label
.endm

/* See more details about the bias in registers.h */
.macro	exception_enter
	sub	sp, sp, #ARCH_EXEC_CONT_SIZE
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	stp	x16, x17, [sp, #16 * 8]
	stp	x18, x19, [sp, #16 * 9]
	stp	x20, x21, [sp, #16 * 10]
	stp	x22, x23, [sp, #16 * 11]
	stp	x24, x25, [sp, #16 * 12]
	stp	x26, x27, [sp, #16 * 13]
	stp	x28, x29, [sp, #16 * 14]
	mrs	x21, sp_el0
	mrs	x22, elr_el1
	mrs	x23, spsr_el1
	stp	x30, x21, [sp, #16 * 15]
	stp	x22, x23, [sp, #16 * 16]
.endm

.macro	exception_exit
	ldp	x22, x23, [sp, #16 * 16]
	ldp	x30, x21, [sp, #16 * 15] 
	msr	sp_el0, x21
	msr	elr_el1, x22
	msr	spsr_el1, x23
	ldp	x0, x1, [sp, #16 * 0]
	ldp	x2, x3, [sp, #16 * 1]
	ldp	x4, x5, [sp, #16 * 2]
	ldp	x6, x7, [sp, #16 * 3]
	ldp	x8, x9, [sp, #16 * 4]
	ldp	x10, x11, [sp, #16 * 5]
	ldp	x12, x13, [sp, #16 * 6]
	ldp	x14, x15, [sp, #16 * 7]
	ldp	x16, x17, [sp, #16 * 8]
	ldp	x18, x19, [sp, #16 * 9]
	ldp	x20, x21, [sp, #16 * 10]
	ldp	x22, x23, [sp, #16 * 11]
	ldp	x24, x25, [sp, #16 * 12]
	ldp	x26, x27, [sp, #16 * 13]
	ldp	x28, x29, [sp, #16 * 14]

	add	sp, sp, #ARCH_EXEC_CONT_SIZE
	eret
.endm

.macro switch_to_cpu_stack
	mrs     x24, TPIDR_EL1
	add	x24, x24, #OFFSET_LOCAL_CPU_STACK
	ldr	x24, [x24]
	mov	sp, x24
.endm

.macro switch_to_thread_ctx
	mrs     x24, TPIDR_EL1
	add	x24, x24, #OFFSET_CURRENT_EXEC_CTX
	ldr	x24, [x24]
	mov	sp, x24
.endm

/*
 * Vector table offsets from vector table base address from ARMv8 Manual
 *	Address		|	Exception Type		| 	Description
 * ============================================================================
 *	VBAR_Eln+0x000	|	 Synchronous		|	 SPSel=0
 * 		+0x080	|	  IRQ/vIRQ		|	Current EL
 *		+0x100	|	  FIQ/vFIQ		|   with Stack Pointer
 * 		+0x180	|	SError/vSError		|    shared with EL0
 * ============================================================================
 *	VBAR_Eln+0x200	|	 Synchronous		|	 SPSel=1
 * 		+0x280	|	  IRQ/vIRQ		|	Current EL
 *		+0x300	|	  FIQ/vFIQ		|   with dedicated
 * 		+0x380	|	SError/vSError		|    Stack Pointer
 * ============================================================================
 *	VBAR_Eln+0x400	|	 Synchronous		|
 * 		+0x480	|	  IRQ/vIRQ		|	Lower EL
 *		+0x500	|	  FIQ/vFIQ		|    using AArch64
 * 		+0x580	|	SError/vSError		|
 * ============================================================================
 *	VBAR_Eln+0x600	|	 Synchronous		|
 * 		+0x680	|	  IRQ/vIRQ		|     	Lower EL
 *		+0x700	|	  FIQ/vFIQ		|    using AArch32
 * 		+0x780	|	SError/vSError		|
 * ============================================================================
 */

/* el1_vector should be set in VBAR_EL1. The last 11 bits of VBAR_EL1 are reserved. */
.align	11
EXPORT(el1_vector)
	exception_entry	sync_el1t		// Synchronous EL1t
	exception_entry	irq_el1t		// IRQ EL1t
	exception_entry	fiq_el1t		// FIQ EL1t
	exception_entry	error_el1t		// Error EL1t

	exception_entry	sync_el1h		// Synchronous EL1h
	exception_entry	irq_el1h		// IRQ EL1h
	exception_entry	fiq_el1h		// FIQ EL1h
	exception_entry	error_el1h		// Error EL1h

	exception_entry	sync_el0_64		// Synchronous 64-bit EL0
	exception_entry	irq_el0_64		// IRQ 64-bit EL0
	exception_entry	fiq_el0_64		// FIQ 64-bit EL0
	exception_entry	error_el0_64		// Error 64-bit EL0

	exception_entry	sync_el0_32		// Synchronous 32-bit EL0
	exception_entry	irq_el0_32		// IRQ 32-bit EL0
	exception_entry	fiq_el0_32		// FIQ 32-bit EL0
	exception_entry	error_el0_32		// Error 32-bit EL0

/*
 * The selected stack pointer can be indicated by a suffix to the Exception Level:
 *  - t: SP_EL0 is used
 *  - h: SP_ELx is used
 *
 * ChCore does not enable or handle irq_el1t, fiq_xxx, and error_xxx.
 * The SPSR_EL1 of idle threads is set to 0b0101, which means interrupt
 * are enabled during the their execution and SP_EL1 is selected (h).
 * Thus, irq_el1h is enabled and handled.
 *
 * Similarly, sync_el1t is also not enabled while we simply reuse the handler for
 * sync_el0 to handle sync_el1h (e.g., page fault during copy_to_user and fpu).
 */

irq_el1h:
        /* Simply reusing exception_enter/exit is OK. */
	exception_enter
#ifndef CHCORE_KERNEL_RT
	switch_to_cpu_stack
#endif
	bl	handle_irq_el1
	/* should never reach here */
	b .

irq_el1t:
fiq_el1t:
fiq_el1h:
error_el1t:
error_el1h:
sync_el1t:
	bl unexpected_handler

sync_el1h:
	exception_enter
	mov	x0, #SYNC_EL1h
	mrs	x1, esr_el1
	mrs	x2, elr_el1
	bl	handle_entry_c
	str     x0, [sp, #16 * 16] /* store the return value as the ELR_EL1 */
	exception_exit

sync_el0_64:
	exception_enter
#ifndef CHCORE_KERNEL_RT
	switch_to_cpu_stack
#endif
	mrs	x25, esr_el1
	lsr	x24, x25, #ESR_EL1_EC_SHIFT
	cmp	x24, #ESR_EL1_EC_SVC_64
	b.eq	el0_syscall
	mov	x0, SYNC_EL0_64 
	mrs	x1, esr_el1
	mrs	x2, elr_el1
	bl	handle_entry_c
#ifdef CHCORE_KERNEL_RT
	bl	do_pending_resched
#else
	switch_to_thread_ctx
#endif
	exception_exit

el0_syscall:

/* hooking syscall: ease tracing or debugging */
#if ENABLE_HOOKING_SYSCALL == ON
	sub	sp, sp, #16 * 8
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	
	mov x0, x8
	bl hook_syscall

	ldp	x0, x1, [sp, #16 * 0]
	ldp	x2, x3, [sp, #16 * 1]
	ldp	x4, x5, [sp, #16 * 2]
	ldp	x6, x7, [sp, #16 * 3]
	ldp	x8, x9, [sp, #16 * 4]
	ldp	x10, x11, [sp, #16 * 5]
	ldp	x12, x13, [sp, #16 * 6]
	ldp	x14, x15, [sp, #16 * 7]
	add	sp, sp, #16 * 8
#endif

	adr	x27, syscall_table		// syscall table in x27
	uxtw	x16, w8				// syscall number in x16
	ldr	x16, [x27, x16, lsl #3]		// find the syscall entry
	blr	x16

	/* Ret from syscall */
	// bl	disable_irq
#ifdef CHCORE_KERNEL_RT
	str	x0, [sp]
	bl	do_pending_resched
#else
	switch_to_thread_ctx
	str	x0, [sp]
#endif
	exception_exit

irq_el0_64:
	exception_enter
#ifndef CHCORE_KERNEL_RT
	switch_to_cpu_stack
#endif
	bl	handle_irq
	/* should never reach here */
	b .

error_el0_64:
fiq_el0_64:
sync_el0_32:
irq_el0_32:
fiq_el0_32:
error_el0_32:
	bl unexpected_handler

/* void eret_to_thread(u64 sp) */
BEGIN_FUNC(__eret_to_thread)
	mov	sp, x0
	dmb ish /* smp_mb() */
#ifdef CHCORE_KERNEL_RT
	bl finish_switch
#endif
	exception_exit
END_FUNC(__eret_to_thread)
