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
#include <irq/timer.h>
#include <common/kprint.h>
#include <common/types.h>
#include <common/macro.h>
#include <common/util.h>
#include <machine.h>
#include <arch/machine/smp.h>
#include <arch/tools.h>
#include <arch/sync.h>
#include <machine.h>
#include <irq/ipi.h>
#include <object/irq.h>
#include "gicv3.h"

/* Wait for completion of a redist change */
static void gic_wait_redist_complete(void)
{
	while (get32(GICR_PER_CPU_BASE(smp_get_cpu_id())) & GICD_CTLR_RWP);
}

/* Wait for completion of a dist change */
static inline void gic_wait_dist_complete(void)
{
	while (get32(GICD_BASE) & GICD_CTLR_RWP);
}

void gicv3_distributor_init(void)
{
	unsigned int gic_type_reg, mpidr, i;
	int nr_lines;
	unsigned long cpumask;

	asm volatile("mrs %0, mpidr_el1\n\t" : "=r"(mpidr));

	gic_type_reg = get32(GICD_TYPER);
	nr_lines = GICD_TYPER_IRQS(gic_type_reg);

	/* Disable the distributor. */
	put32(GICD_CTLR, 0);
	gic_wait_dist_complete();

	for (i = 32; i < nr_lines; i += 32) {
		/* Diactivate the corresponding interrupt. */
		put32(GICD_ICACTIVER + (i >> 3), GICD_ICACTIVER_DEFAULT_MASK);
		/* Disables forwarding of the corresponding interrupt. */
		put32(GICD_ICENABLER + (i >> 3), GICD_ICENABLER_DEFAULT_MASK);
		/*
		 * Changes the state of the corresponding interrupt from pending to inactive,
		 * or from active and pending to active.
		 */
		put32(GICD_ICPENDR + (i >> 3), GICD_ICPENDR_DEFAULT_MASK);
		/* Configure the corresponding interrupt as non-secure Group-1. */
		put32(GICD_IGROUPR + (i >> 3), GICD_IGROUPR_DEFAULT_MASK);
		put32(GICD_IGRPMODR + (i >> 3), GICD_IGRPMODR_DEFAULT_MASK);
	}

	gic_wait_dist_complete();

	/* Default priority for global interrupts */
	for (i = 32; i < nr_lines; i += 4) {
		put32(GICD_IPRIORITYR + i, GICD_INT_DEF_PRI_X4);
	}

	/* Default all global IRQs to level, active low */
	for (i = 32; i < nr_lines; i += 16) {
		put32(GICD_ICFGR + (i >> 2), GICD_INT_ACTLOW_LVLTRIG);
	}

	gic_wait_dist_complete();

	/* Enable distributor with ARE, Group1 */
	put32(GICD_CTLR, GICD_CTLR_ARE_NS | GICD_CTLR_ENABLE_G1_NS);

	/* Refers to GICv3 manual GICD_IROUTER register */
	cpumask = ((mpidr >> 0) & 0xff) |
		  (((mpidr >> 8) & 0xff) << 8) |
		  (((mpidr >> 16) & 0xff) << 16) |
		  ((((unsigned long)mpidr >> 24) & 0xff) << 32);
	for (i = 32; i < nr_lines; i++)
		gicv3_set_routing(cpumask, (void *)(GICD_IROUTER + i * 8));
}

static void gicv3_init_cpu_interface(void)
{
	bool group0;
	unsigned int val, pribits;

	/* Make sure that the SRE bit has been set. */
	if (!gicv3_enable_sre()) {
		BUG("GICv3 fatal error: SRE bit not set (disabled at EL2)\n");
		return;
	}

	pribits = read_sys_reg(ICC_CTLR_EL1);
	pribits &= ICC_CTLR_EL1_PRIbits_MASK;
	pribits >>= ICC_CTLR_EL1_PRIbits_SHIFT;
	pribits++;

	/*
	 * Let's find out if Group0 is under control of EL3 or not by
	 * setting the highest possible, non-zero priority in PMR.
	 *
	 * If SCR_EL3.FIQ is set, the priority gets shifted down in
	 * order for the CPU interface to set bit 7, and keep the
	 * actual priority in the non-secure range. In the process, it
	 * looses the least significant bit and the actual priority
	 * becomes 0x80. Reading it back returns 0, indicating that
	 * we're don't have access to Group0.
	 */
	write_sys_reg(ICC_PMR_EL1, BIT(8 - pribits));
	val = read_sys_reg(ICC_PMR_EL1);
	group0 = val != 0;

	write_sys_reg(ICC_PMR_EL1, DEFAULT_PMR_VALUE);

	/*
	 * Some firmwares hand over to the kernel with the BPR changed from
	 * its reset value (and with a value large enough to prevent
	 * any pre-emptive interrupts from working at all). Writing a zero
	 * to BPR restores is reset value.
	 */
	write_sys_reg(ICC_BPR1_EL1, 0);

	/* EOI drops priority and deactivates interrupt. */
	write_sys_reg(ICC_CTLR_EL1, (1 << ICC_CTLR_EL1_EOImode_SHIFT) | (1 << ICC_CTLR_EL1_CBPR_SHIFT));
	isb();

	/* Always check Group0 before Group1. */
	if (group0) {
		switch (pribits) {
		case 8:
		case 7:
			write_sys_reg(ICC_AP0R3_EL1, 0);
			write_sys_reg(ICC_AP0R2_EL1, 0);
		/* Fall through */
		case 6:
			write_sys_reg(ICC_AP0R1_EL1, 0);
		/* Fall through */
		case 5:
		case 4:
			write_sys_reg(ICC_AP0R0_EL1, 0);
			break;
		default:
			kwarn("pribits(value: %d) may be wrong!\n", pribits);
			break;
		}
		isb();
	}

	switch (pribits) {
	case 8:
	case 7:
		write_sys_reg(ICC_AP1R3_EL1, 0);
		write_sys_reg(ICC_AP1R2_EL1, 0);
	/* Fall through */
	case 6:
		write_sys_reg(ICC_AP1R1_EL1, 0);
	/* Fall through */
	case 5:
	case 4:
		write_sys_reg(ICC_AP1R0_EL1, 0);
		break;
	default:
		kwarn("pribits(value: %d) may be wrong!\n", pribits);
		break;
	}
	isb();

	write_sys_reg(ICC_IGRPEN1_EL1, 1);
	isb();
}

static void gicv3_redistributor_init(void)
{
	unsigned int gic_wake_reg;
	unsigned long redis_base, sgi_base;

	redis_base = GICR_PER_CPU_BASE(smp_get_cpu_id());

	/* Clear processor sleep and wait till childasleep is cleard. */
	gic_wake_reg = get32(redis_base + GICR_WAKER_OFFSET);
	gic_wake_reg &= ~GICR_WAKER_ProcessorSleep;
	put32(redis_base + GICR_WAKER_OFFSET, gic_wake_reg);
	while (get32(redis_base + GICR_WAKER_OFFSET) & GICR_WAKER_ChildrenAsleep);

	sgi_base = redis_base + GICR_SGI_BASE_OFFSET;

	/* Diactivate SGIs and PPIs. */
	put32(sgi_base + GICR_ICACTIVER0_OFFSET, GICR_ICACTIVER0_DEFAULT_MASK);
	/* Disable forwarding of PPIs. */
	put32(sgi_base + GICR_ICENABLER0_OFFSET, GICR_ICENABLER0_DEFAULT_MASK);
	/* Enable forwarding of SGIs. */
	put32(sgi_base + GICR_ISENABLER0_OFFSET, GICR_ISENABLER0_DEFAULT_MASK);
	/* Configure SGIs and PPIs as Non-secure Group-1. */
	put32(sgi_base + GICR_IGROUPR0_OFFSET, GICR_IGROUPR0_DEFAULT_MASK);

	/* Set priority for SGIs and PPIs. */
	for (int i = 0; i < 32; i += 4)
		put32(sgi_base + GICR_IPRIORITYR_OFFSET + i, GICD_INT_DEF_PRI_X4);

	/* Set SGIs and PPIs as level-sensitive. */
	put32(sgi_base + GICR_ICFGR0_OFFSET, 0);
	put32(sgi_base + GICR_ICFGR1_OFFSET, 0);

	gic_wait_redist_complete();

	/* Initialise cpu interface registers. */
	gicv3_init_cpu_interface();
}

void plat_enable_irqno(int irq)
{
	int cpu_id;

	if (irq < 32) {
		/* irq in redistributor. */
		cpu_id = smp_get_cpu_id();
		put32(GICR_PER_CPU_BASE(cpu_id) + GICR_SGI_BASE_OFFSET
		      + GICR_ISENABLER0_OFFSET,
		      (1 << (irq % 32)));
		gic_wait_redist_complete();
	} else {
		/* irq in distributor. */
		put32(GICD_BASE + GICD_ISENABLER_OFFSET + ((irq >> 5) << 2),
		      (1 << (irq % 32)));
		gic_wait_dist_complete();
	}
}

void plat_disable_irqno(int irq)
{
	int cpu_id;

	if (irq < 32) {
		// irq in redist
		cpu_id = smp_get_cpu_id();
		put32(GICR_PER_CPU_BASE(cpu_id) + GICR_SGI_BASE_OFFSET
		      + GICR_ICENABLER0_OFFSET,
		      (1 << (irq % 32)));
		gic_wait_redist_complete();
	} else {
		// irq in dist
		put32(GICD_BASE + GICD_ICENABLER_OFFSET + ((irq >> 5) << 2),
		      (1 << (irq % 32)));
		gic_wait_dist_complete();
	}
}

void plat_interrupt_init(void)
{
	unsigned int cpuid = smp_get_cpu_id();

	if (cpuid == 0)
		gicv3_distributor_init();

	/* Init the cpu interface (GICC) */
	gicv3_redistributor_init();

	/* Enable timer irq */
	plat_enable_irqno(GIC_INTID_PPI_EL1_PHYS_TIMER);
}

unsigned long mpidr_map[PLAT_CPU_NUM] = {
	0x0,
	0x1,
	0x100,
	0x101,
#ifdef CHCORE_SUBPLAT_D2000V
	0x200,
	0x201,
	0x300,
	0x301
#endif
};

void plat_send_ipi(unsigned int cpu, unsigned int ipi)
{
	unsigned long val;
	unsigned long cluster_id = MPIDR_TO_SGI_CLUSTER_ID(mpidr_map[cpu]);
	unsigned short tlist;

	BUG_ON(cpu >= PLAT_CPU_NUM);
	tlist = 1 << (mpidr_map[cpu] & 0xf);

	val = (MPIDR_TO_SGI_AFFINITY(cluster_id, 3)
	       | MPIDR_TO_SGI_AFFINITY(cluster_id, 2)
	       | ipi << ICC_SGI1R_SGI_ID_SHIFT
	       | MPIDR_TO_SGI_AFFINITY(cluster_id, 1)
	       | MPIDR_TO_SGI_RS(cluster_id)
	       | tlist);

	write_sys_reg(ICC_SGI1R_EL1, val);
	isb();
}

void plat_ack_irq(int irq)
{
	plat_enable_irqno(irq);
}

/* Note that rescheduling happens after plat_handle_irq */
void plat_handle_irq(void)
{
	unsigned int irqnr = 0;
	unsigned int irqstat = 0;
	int ret;
	bool handled = false;

	irqstat = read_sys_reg(ICC_IAR1_EL1);
	dsb(sy);
	irqnr = irqstat & 0x3ff;

	/*
	 * We only support 0~15 SGI, 16~31 PPI and 32~1019 SPI.
	 * 8192 ~ MAX LPIs not supported fow now.
	 */
	if (likely(irqnr > 15 && irqnr < 1020)) {
		switch (irqnr) {
		case (GIC_INTID_PPI_EL1_PHYS_TIMER):
			/* EL1 physical timer */
			handle_timer_irq();
			handled = true;
			break;
		default:
			/* Likely to be handled by user */
			break;
		}
	} else if (irqnr < 16) {
		handle_ipi();
		handled = true;
	}

	if (irqnr >= 1020 && irqnr <= 1023) {
		/* Ignore platform specific irq. */
		handled = true;
	}

	write_sys_reg(ICC_EOIR1_EL1, irqstat);
	write_sys_reg(ICC_DIR_EL1, irqstat);
	isb();

	if (!handled) {
		BUG_ON(irqnr >= MAX_IRQ_NUM);
		if (likely(irq_handle_type[irqnr] == HANDLE_USER)) {
			kdebug("%s: irqnr=%d to user\n", __func__, irqnr);
			plat_disable_irqno(irqnr);
			ret = user_handle_irq(irqnr);
			BUG_ON(ret);
		} else {
			kinfo("Unhandled IRQ 0x%x\n", irqstat);
			BUG_ON(1);
		}
	}
}
