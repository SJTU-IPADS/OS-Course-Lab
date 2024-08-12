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
#include <arch/sync.h>
#include <irq/ipi.h>
#include <object/irq.h>

/*
 * Refer to BROADCOM BCM2835 manual (no official BCM2837 manual).
 * The IRQ details are the same on 2835 and 2837.
 */
#define IRQ_USB            (9)
#define IRQ_USB_BIT        (1 << IRQ_USB)
#define IRQ_SDIO           (32 + 24)
#define IRQ_SDIO_BIT       (1 << (IRQ_SDIO - 32))
#define IRQ_UART           (32 + 25)
#define IRQ_UART_BIT       (1 << (IRQ_UART - 32))
#define IRQ_ARASANSDIO     (32 + 30)
#define IRQ_ARASANSDIO_BIT (1 << (IRQ_ARASANSDIO - 32))

/* Per core IRQ SOURCE MMIO address */
u64 core_irq_source[PLAT_CPU_NUM] = {
	CORE0_IRQ_SOURCE,
	CORE1_IRQ_SOURCE,
	CORE2_IRQ_SOURCE,
	CORE3_IRQ_SOURCE,
};

static void bcm2835_ipi_init(void)
{
	u32 cpuid;
	cpuid = smp_get_cpu_id();
	/* enable the 1st mailbox as ipi irq */
	put32(CORE_MBOX_CTL(cpuid), BIT(0));
}

static void interrupt_init(void)
{
	static int once = 1;

	if (once == 1) {
		once = 0;

		/* Disable IRQs */
		put32(BCM2835_IRQ_FIQ_CTRL, 0);

		put32(BCM2835_IRQ_DISABLE1, (u32)-1);
		put32(BCM2835_IRQ_DISABLE2, (u32)-1);
		put32(BCM2835_IRQ_DISABLE_BASIC, (u32)-1);

		/* ACK pending IRQs. TODO (RO reg?) */
		put32(BCM2835_IRQ_BASIC, get32(BCM2835_IRQ_BASIC));
		put32(BCM2835_IRQ_PENDING1, get32(BCM2835_IRQ_PENDING1));
		put32(BCM2835_IRQ_PENDING2, get32(BCM2835_IRQ_PENDING2));

		/* Enable USB interrupt: writing a 1 does not affect other bits. */
		// put32(BCM2835_IRQ_ENABLE1, (1 << 9));

		/*
		put32(BCM2835_IRQ_ENABLE1, (1 << 1));
		put32(BCM2835_IRQ_ENABLE_BASIC, (1 << 0));
		put32(BCM2835_IRQ_ENABLE_BASIC, (1 << 1));
		*/

		#if USE_mini_uart == 0
		/* Enable uart (pl011) irq */
		enable_uart_irq(IRQ_UART);
		#endif

		isb();
		smp_mb();
	}
}

void plat_interrupt_init(void)
{
	bcm2835_ipi_init();

	interrupt_init();
}

void plat_send_ipi(u32 cpu, u32 ipi)
{
	BUG_ON(cpu >= PLAT_CPU_NUM);
	BUG_ON(ipi >= 32);
	put32(CORE_MBOX_SET(cpu, 0), 1 << ipi);
}

static inline void plat_handle_ipi(void)
{
	u32 cpuid, mbox, ipi;

	cpuid = smp_get_cpu_id();
	mbox = get32(CORE_MBOX_RDCLR(cpuid, 0));
	ipi = ctzl(mbox);
	put32(CORE_MBOX_RDCLR(cpuid, 0), 1 << ipi);
	handle_ipi();
}

void plat_enable_irqno(int irq)
{
	if (irq < 32)
		put32(BCM2835_IRQ_ENABLE1, (1 << irq));
	else if (irq < 64)
		put32(BCM2835_IRQ_ENABLE2, (1 << (irq - 32)));

	/*
	 * TODO: Base Interrupt enable register for other IRQs.
	 * Not all 0-64 irq numbers are legal.
	 */
}

void plat_disable_irqno(int irq)
{
	if (irq < 32)
		put32(BCM2835_IRQ_DISABLE1, (1 << irq));
	else if (irq < 64)
		put32(BCM2835_IRQ_DISABLE2, (1 << (irq - 32)));

	/*
	 * TODO: Base Interrupt enable register for other IRQs.
	 * Not all 0-64 irq numbers are legal.
	 */
}


/* TODO: irq notification in raspi3 */
void plat_ack_irq(int irq)
{
	/* empty */
}

void plat_handle_irq(void)
{
	u32 cpuid = 0;
	unsigned int irq_src, irq;

	cpuid = smp_get_cpu_id();
	irq_src = get32(core_irq_source[cpuid]);

	/* Handle one irq one time. Handle ipi first */
	if (irq_src & INT_SRC_MBOX0) {
		plat_handle_ipi();
		return;
	}

	irq = 1 << ctzl(irq_src);
	switch (irq) {
	case INT_SRC_TIMER1:
		/* CNTPNSIRQ (Physical Non-Secure timer IRQ) */
		// kinfo("handle_timer_irq\n");
		handle_timer_irq();
		return;
	default:
		// kinfo("Unsupported IRQ %d\n", irq);
		break;
	}

	/* By default, interrupts are routed to CPU 0. */
	BUG_ON(cpuid != 0);

	// TODO: handle all irqs?
	irq = get32(BCM2835_IRQ_PENDING1);
	if (irq != 0) {
		if (irq & IRQ_USB_BIT) {
			if (irq_handle_type[IRQ_USB] == HANDLE_USER) {
				/* Go to user space */
				user_handle_irq(IRQ_USB);
			}
		}
	}
	irq = get32(BCM2835_IRQ_PENDING2);
	if (irq != 0) {
		if (irq & IRQ_SDIO_BIT) {
			if (irq_handle_type[IRQ_SDIO] == HANDLE_USER) {
				user_handle_irq(IRQ_SDIO);
			}
		} else if (irq & IRQ_ARASANSDIO_BIT) {
			if (irq_handle_type[IRQ_ARASANSDIO] == HANDLE_USER) {
				user_handle_irq(IRQ_ARASANSDIO);
			}
		}

		#if USE_mini_uart == 0
		if (irq & IRQ_UART_BIT) {
			uart_irq_handler();

			if (irq_handle_type[IRQ_UART] == HANDLE_USER)
				user_handle_irq(IRQ_UART);
		}
		#endif /* By default, PL011 is used */
	}

	return;
}
