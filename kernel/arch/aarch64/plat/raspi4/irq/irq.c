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
#include <common/kprint.h>
#include <irq/timer.h>
#include <io/uart.h>
#include <machine.h>
#include <common/types.h>
#include <common/macro.h>
#include <common/bitops.h>
#include <arch/machine/smp.h>
#include <arch/tools.h>
#include <irq/ipi.h>
#include <object/irq.h>

/* TODO (tmac): rewrite this file */

static void gicv2_dist_init(void)
{
	u32 type = 0;
	u32 cpumask = 0;
	// u32 gic_cpus = 0;
	unsigned int nr_lines;
	int i;

	cpumask = get32(GICD_ITARGETSR) & 0xff;
	cpumask |= cpumask << 8;
	cpumask |= cpumask << 16;

	/* Disable the distributor */
	put32(GICD_CTLR, 0);

	type = get32(GICD_TYPER);
	nr_lines = 32 * ((type & GICD_TYPE_LINES) + 1);

	// gic_cpus = 1 + ((type & GICD_TYPE_CPUS) >> 5);
	kdebug("GICv2: %d lines, %d cpu%s%s (IID %8.8x).\n",
	       nr_lines, 1 + ((type & GICD_TYPE_CPUS) >> 5),
	       (1 + ((type & GICD_TYPE_CPUS) >> 5) == 1) ? "" : "s",
	       (type & GICD_TYPE_SEC) ? ", secure" : "", get32(GICD_IIDR));

	/* Default all global IRQs to level, active low */
	for (i = 32; i < nr_lines; i += 16)
		put32(GICD_ICFGR + (i / 16) * 4, 0x0);

	/* Route all global IRQs to this CPU */
	for (i = 32; i < nr_lines; i += 4)
		put32(GICD_ITARGETSR + (i / 4) * 4, cpumask);

	/* Default priority for global interrupts */
	for (i = 32; i < nr_lines; i += 4)
		put32(GICD_IPRIORITYR + (i / 4) * 4,
		      GIC_PRI_IRQ << 24 | GIC_PRI_IRQ << 16 |
		      GIC_PRI_IRQ << 8 | GIC_PRI_IRQ);

	/* Disable all global interrupts */
	for (i = 32; i < nr_lines; i += 32) {
		put32(GICD_ICENABLER + (i / 32) * 4, ~0x0);
		put32(GICD_ICACTIVER + (i / 32) * 4, ~0x0);
	}

	/* Turn on the distributor */
	put32(GICD_CTLR, GICD_CTL_ENABLE);
}

static void gicv2_cpu_init(void)
{
	int i;

	put32(GICD_ICACTIVER, 0xffffffff);	/* Diactivate PPIs and SPIs */
	put32(GICD_ICENABLER, 0xffff0000);	/* Disable all PPI */
	/*
	 * Writes to SGI bits in gic-400 are ignored.
	 * FIXME: SGI 8 is not enabled.
	 */
	/*
	 * TODO(MK): There are more than 32 bits to handle.
	 */
#if 0
	int nr_interrupts = ((get32(GICD_TYPER) & 0b11111) + 1) * 32;
	/* Enable all PPI */
	for (i = 0; i < nr_interrupts; i += 32) {
		put32(GICD_ISENABLER + 0x4 * i/32, 0xffffffff);
	}
#endif

	/* Set SGI priorities */
	for (i = 0; i < 16; i += 4)
		put32(GICD_IPRIORITYR + (i / 4) * 4,
		      GIC_PRI_IPI << 24 | GIC_PRI_IPI << 16 |
		      GIC_PRI_IPI << 8 | GIC_PRI_IPI);

	/* Set PPI priorities */
	for (i = 16; i < 32; i += 4)
		put32(GICD_IPRIORITYR + (i / 4) * 4,
		      GIC_PRI_IRQ << 24 | GIC_PRI_IRQ << 16 |
		      GIC_PRI_IRQ << 8 | GIC_PRI_IRQ);

	/* Don't mask by priority */
	put32(GICC_PMR, 0xff);
	/* Finest granularity of priority */
	put32(GICC_BPR, 0x0);
	/* turn on delivery */
	put32(GICC_CTLR, GICC_CTL_ENABLE | GICC_CTL_EOI);
}

void plat_interrupt_init(void)
{
	kinfo("enter plat_interrupt_init\n");
	u32 cpuid = smp_get_cpu_id();

	if (cpuid == 0)
		gicv2_dist_init();

	/* init the cpu interface (GICC) */
	gicv2_cpu_init();

	/* enable the timer's irq */
	put32(GICD_ISENABLER, (1 << GIC_INTID_PPI_EL1_PHYS_TIMER) |
				      (1 << GIC_INTID_PPI_EL1_VIRT_TIMER));

	/* Enable interrrupt for UFS */
	/* Why do we need to add 32? */
	// u32 intid = 0x116 + 32;
	// put32(GICD_ISENABLER + 0x4*(intid/32), 1ul<<(intid%32));
	// put32(GICD_ISENABLER, 1ul<<14);
}

void plat_enable_irqno(int irq)
{
	put32(GICD_ISENABLER + 0x4*(irq/32), 1ul<<(irq%32));
}

void plat_disable_irqno(int irq)
{
	put32(GICD_ICENABLER + 0x4*(irq/32), 1ul<<(irq%32));
}

void plat_send_ipi(u32 cpu, u32 ipi)
{
	BUG_ON(cpu >= PLAT_CPU_NUM);
	put32(GICD_SGIR, GICD_SGIR_VAL(0, 1 << cpu, ipi));
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
	int r;
	bool handled = true;

	irqstat = get32(GICC_IAR);
	irqnr = irqstat & 0x3ff;

	switch (irqnr) {
	case GIC_INTID_PPI_EL1_PHYS_TIMER:
		/* EL1 physical timer */
		handle_timer_irq();
		break;
	case IPI_RESCHED:
		handle_ipi();
		break;
	default:
		handled = false;
		break;
	}
	put32(GICC_EOIR, irqstat);
	put32(GICC_DIR, irqstat);

	if (!handled) {
		BUG_ON(irqnr >= MAX_IRQ_NUM);
		if (irq_handle_type[irqnr] == HANDLE_USER) {
			r = user_handle_irq(irqnr);
			BUG_ON(r);
		}
	}
}
