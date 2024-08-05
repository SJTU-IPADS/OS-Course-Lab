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

#include <irq/irq.h>
#include <common/types.h>
#include <common/kprint.h>
#include <common/util.h>
#include <sched/sched.h>
#include <sched/fpu.h>
#include <arch/machine/smp.h>
#include <arch/machine/esr.h>
#include "irq_entry.h"

u8 irq_handle_type[MAX_IRQ_NUM];

void arch_enable_irqno(int irq)
{
	plat_enable_irqno(irq);
}

void arch_disable_irqno(int irq)
{
	plat_disable_irqno(irq);
}

void arch_interrupt_init_per_cpu(void)
{
	disable_irq();

	/* platform dependent init */
	set_exception_vector();
	plat_interrupt_init();
}

void arch_interrupt_init(void)
{
	arch_interrupt_init_per_cpu();
	memset(irq_handle_type, HANDLE_KERNEL, MAX_IRQ_NUM);
}

u64 handle_entry_c(int type, u64 esr, u64 address)
{
	/* ec: exception class */
	u32 esr_ec = GET_ESR_EL1_EC(esr);

	kdebug("Exception type: %d, ESR: 0x%lx, Fault address: 0x%lx, "
	       "EC 0b%b\n", type, esr, address, esr_ec);

	/* Currently, ChCore only handles a part of IRQs */
	if (type < SYNC_EL0_64) {

		if (esr_ec != ESR_EL1_EC_DABT_CEL) {
			kinfo("%s: irq type is %d\n", __func__, type);
			BUG_ON(1);
		}
	}

	u64 fix_addr = 0;
	/* Dispatch exception according to EC */
	switch (esr_ec) {
	case ESR_EL1_EC_UNKNOWN:
		kdebug("Unknown\n");
		break;
	case ESR_EL1_EC_WFI_WFE:
		kdebug("Trapped WFI or WFE instruction execution\n");
		return address;
	case ESR_EL1_EC_ENFP:
#if FPU_SAVING_MODE == LAZY_FPU_MODE
		/*
		 * Disable FPU in EL1: IRQ type is SYNC_EL1h (4).
		 * Disable FPU in EL0: IRQ type is SYNC_EL0_64 (8).
		 */
		change_fpu_owner();
		return address;
#else
		kdebug("Access to SVE, Advanced SIMD, or floating-point functionality\n");
		break;
#endif
	case ESR_EL1_EC_ILLEGAL_EXEC:
		kdebug("Illegal Execution state\n");
		break;
	case ESR_EL1_EC_SVC_32:
		kdebug("SVC instruction execution in AArch32 state\n");
		break;
	case ESR_EL1_EC_SVC_64:
		kdebug("SVC instruction execution in AArch64 state\n");
		break;
	case ESR_EL1_EC_MRS_MSR_64:
		kdebug("Using MSR or MRS from a lower Exception level\n");
		break;
	case ESR_EL1_EC_IABT_LEL:
		kdebug("Instruction Abort from a lower Exception level\n");
		/* Page fault handler here:
		 * dynamic loading can trigger faults here.
		 */
		do_page_fault(esr, address, type, &fix_addr);
		return address;
	case ESR_EL1_EC_IABT_CEL:
		kinfo("Instruction Abort from current Exception level\n");
		break;
	case ESR_EL1_EC_PC_ALIGN:
		kdebug("PC alignment fault exception\n");
		break;
	case ESR_EL1_EC_DABT_LEL:
		kdebug("Data Abort from a lower Exception level\n");
		/* Handle faults caused by data access.
		 * We only consider page faults for now.
		 */
		do_page_fault(esr, address, type, &fix_addr);
		return address;
	case ESR_EL1_EC_DABT_CEL:
		kdebug("Data Abort from a current Exception level\n");
		do_page_fault(esr, address, type, &fix_addr);

		if (fix_addr)
			return fix_addr;
		else 
			return address;
	case ESR_EL1_EC_SP_ALIGN:
		kdebug("SP alignment fault exception\n");
		break;
	case ESR_EL1_EC_FP_32:
		kdebug("Trapped floating-point exception taken from AArch32 state\n");
		break;
	case ESR_EL1_EC_FP_64:
		kdebug("Trapped floating-point exception taken from AArch64 state\n");
		break;
	case ESR_EL1_EC_SError:
		kdebug("SERROR\n");
		break;
	default:
		kdebug("Unsupported Exception ESR %lx\n", esr);
		break;
	}

	BUG("Exception type: %d, ESR: 0x%lx, Fault address: 0x%lx, "
	    "EC 0b%b\n", type, esr, address, esr_ec);
	__builtin_unreachable();
}

/* Interrupt handler for interrupts happening when in EL0. */
void handle_irq(void)
{
	plat_handle_irq();
	eret_to_thread(switch_context());
}

/* Interrupt handler for interrupts happening when in EL1 (idle thread). */
void handle_irq_el1(u64 elr_el1)
{
	/* Just reuse the same interrupt handler for EL0. */
	handle_irq();
}

void unexpected_handler(void)
{
	BUG("[fatal error] %s is invoked\n", __func__);
	__builtin_unreachable();
}
