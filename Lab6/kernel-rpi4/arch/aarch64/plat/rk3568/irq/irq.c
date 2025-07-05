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

#include <arch/tools.h>
#include <arch/mmu.h>
#include <arch/machine/smp.h>
#include <common/types.h>
#include <common/macro.h>
#include <mm/mm.h>

/* Maximum number of interrups a GIC can support */
#define GIC_MAX_INTS 1020

/* Number of interrupts in one register */
#define NUM_INTS_PER_REG 32

/* Offsets from gic.gicd_base */
#define GICD_CTLR          (0x000)
#define GICD_TYPER         (0x004)
#define GICD_IGROUPR(n)    (0x080 + (n)*4)
#define GICD_ISENABLER(n)  (0x100 + (n)*4)
#define GICD_ICENABLER(n)  (0x180 + (n)*4)
#define GICD_ISPENDR(n)    (0x200 + (n)*4)
#define GICD_ICPENDR(n)    (0x280 + (n)*4)
#define GICD_IPRIORITYR(n) (0x400 + (n)*4)
#define GICD_ITARGETSR(n)  (0x800 + (n)*4)
#define GICD_IGROUPMODR(n) (0xd00 + (n)*4)
#define GICD_SGIR          (0xF00)

#define GICD_CTLR_ENABLEGRP1S (1 << 2)

static inline u32 read_icc_ctlr(void)
{
        u64 val64 = 0;
        asm volatile("mrs %0, "
                     "S3_0_C12_C12_4"
                     : "=r"(val64));
        return val64;
}

static inline void write_icc_ctlr(u32 val)
{
        u64 val64 = val;
        asm volatile("msr "
                     "S3_0_C12_C12_4"
                     ", %0"
                     :
                     : "r"(val64));
}

static inline void write_icc_pmr(u32 val)
{
        u64 val64 = val;
        asm volatile("msr "
                     "S3_0_C4_C6_0"
                     ", %0"
                     :
                     : "r"(val64));
}

static inline void write_icc_igrpen1(u32 val)
{
        u64 val64 = val;
        asm volatile("msr "
                     "S3_0_C12_C12_7"
                     ", %0"
                     :
                     : "r"(val64));
}

static inline void set32(vaddr_t addr, u32 set_mask)
{
        put32(addr, get32(addr) | set_mask);
}

static size_t probe_max_it(vaddr_t gicd_base)
{
        int i;
        u32 old_ctlr;
        size_t ret = 0;
        const size_t max_regs =
                ((GIC_MAX_INTS + NUM_INTS_PER_REG - 1) / NUM_INTS_PER_REG) - 1;

        /*
         * Probe which interrupt number is the largest.
         */
        old_ctlr = read_icc_ctlr();
        write_icc_ctlr(0);
        for (i = max_regs; i >= 0; i--) {
                u32 old_reg;
                u32 reg;
                int b;

                old_reg = get32(gicd_base + GICD_ISENABLER(i));
                put32(gicd_base + GICD_ISENABLER(i), 0xffffffff);
                reg = get32(gicd_base + GICD_ISENABLER(i));
                put32(gicd_base + GICD_ICENABLER(i), ~old_reg);
                for (b = NUM_INTS_PER_REG - 1; b >= 0; b--) {
                        if ((u32)BIT(b) & reg) {
                                ret = i * NUM_INTS_PER_REG + b;
                                goto out;
                        }
                }
        }
out:
        write_icc_ctlr(old_ctlr);
        return ret;
}

void __plat_interrupt_init_percpu(vaddr_t gicd_base)
{
        /* per-CPU interrupts config:
         * ID0-ID7(SGI)   for Non-secure interrupts
         * ID8-ID15(SGI)  for Secure interrupts.
         * All PPI config as Non-secure interrupts.
         */
        put32(gicd_base + GICD_IGROUPR(0), 0xffff00ff);

        /* Set the priority mask to permit Non-secure interrupts, and to
         * allow the Non-secure world to adjust the priority mask itself
         */
        write_icc_pmr(0x80);
        write_icc_igrpen1(1);
}

void __plat_interrupt_init(vaddr_t gicd_base)
{
        size_t n, max_it;

        max_it = probe_max_it(gicd_base);

        for (n = 0; n <= max_it / NUM_INTS_PER_REG; n++) {
                /* Disable interrupts */
                put32(gicd_base + GICD_ICENABLER(n), 0xffffffff);

                /* Make interrupts non-pending */
                put32(gicd_base + GICD_ICPENDR(n), 0xffffffff);

                /* Mark interrupts non-secure */
                if (n == 0) {
                        /* per-CPU inerrupts config:
                         * ID0-ID7(SGI)	  for Non-secure interrupts
                         * ID8-ID15(SGI)  for Secure interrupts.
                         * All PPI config as Non-secure interrupts.
                         */
                        put32(gicd_base + GICD_IGROUPR(n), 0xffff00ff);
                } else {
                        put32(gicd_base + GICD_IGROUPR(n), 0xffffffff);
                }
        }

        write_icc_pmr(0x80);
        write_icc_igrpen1(1);
        set32(gicd_base + GICD_CTLR, GICD_CTLR_ENABLEGRP1S);
}

void plat_interrupt_init(void)
{
        vaddr_t gicd_base;

        gicd_base = phys_to_virt(get_gicd_base());

        if (smp_get_cpu_id() == 0) {
                __plat_interrupt_init(gicd_base);
        } else {
                __plat_interrupt_init_percpu(gicd_base);
        }
}

void plat_send_ipi(u32 cpu, u32 ipi)
{
}

void plat_enable_irqno(int irq)
{
}

void plat_disable_irqno(int irq)
{
}

void plat_ack_irq(int irq)
{
}

void plat_handle_irq(void)
{
}
