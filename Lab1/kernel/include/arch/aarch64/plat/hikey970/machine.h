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

#ifndef ARCH_AARCH64_PLAT_HIKEY970_MACHINE_H
#define ARCH_AARCH64_PLAT_HIKEY970_MACHINE_H

#include <common/vars.h>

/* hikey970 config */
#define PLAT_CPU_NUM            8
#define PLAT_PROCESSOR_NUM      2
#define PLAT_CORE_NUM           4
#define PLAT_HIKEY

#define CONSOLE_BAUDRATE        115200
#define CONSOLE_UART_CLK_IN_HZ  19200000
#define UARTDR      0x000
#define UARTFR      0x018
#define UARTFR_TXFF (1 << 5)
/*
#define PL011_UART0_BASE    0xF8015000
#define PL011_UART1_BASE    0xF7111000
#define PL011_UART2_BASE    0xF7112000
#define PL011_UART3_BASE    0xF7113000
#define PL011_UART4_BASE    0xF7114000
*/
#define PL011_UART6_BASE    (KBASE + 0xFFF32000)
#define PL011_REG_SIZE	0x1000

/* This address comes from device tree: hisilicon/hi3670.dtsi */
#define GIC_BASE (KBASE + 0xE82B0000)

#define GICD_BASE (GIC_BASE + 0x1000)
#define GICC_BASE (GIC_BASE + 0x2000)
#define GICH_BASE (GIC_BASE + 0x4000)
#define GICV_BASE (GIC_BASE + 0x6000)
#define GICD_SIZE 0x1000
#define GICC_SIZE 0x2000
#define GICH_SIZE 0x2000
#define GICV_SIZE 0x2000

/*
 * Offsets of GICH Registers.
 *
 * Use offset instead of absolute address
 * because we need to use `ldr w5, [x2, GICH_HCR]`
 * style assembly to save and restore these registers,
 * where x2 is the base address of GICH register block.
 *
 * TODO: Accessing GICD and GICC registers should also
 * be in this fashion too.
 */
#define GICH_HCR    0x0
#define GICH_VTR    0x4
#define GICH_VMCR   0x8
#define GICH_MISR   0x10
#define GICH_EISR0  0x20
#define GICH_EISR1  0x24
#define GICH_ELRSR0 0x30
#define GICH_ELRSR1 0x34
#define GICH_APR    0xf0
#define GICH_LR0    0x100

/* GICD Registers */
#define GICD_CTLR	(GICD_BASE+0x000)
#define GICD_TYPER	(GICD_BASE+0x004)
#define GICD_IIDR	(GICD_BASE+0x008)
#define GICD_ISENABLER	(GICD_BASE+0x100)
#define GICD_ICENABLER	(GICD_BASE+0x180)
#define GICD_ICPENDR	(GICD_BASE+0x280)
#define GICD_ISACTIVER	(GICD_BASE+0x300)
#define GICD_ICACTIVER	(GICD_BASE+0x380)
#define GICD_IPRIORITYR	(GICD_BASE+0x400)
#define GICD_ITARGETSR	(GICD_BASE+0x800)
#define GICD_ICFGR	(GICD_BASE+0xC00)
#define GICD_PPISR	(GICD_BASE+0xD00)
#define GICD_SGIR	(GICD_BASE+0xF00)
#define GICD_SGIR_CLRPEND	(GICD_BASE+0xF10)
#define GICD_SGIR_SETPEND	(GICD_BASE+0xF20)

/* GICC Registers */
#define GICC_CTLR	(GICC_BASE+0x0000)
#define GICC_PMR	(GICC_BASE+0x0004)
#define GICC_BPR	(GICC_BASE+0x0008)
#define GICC_IAR	(GICC_BASE+0x000C)
#define GICC_EOIR	(GICC_BASE+0x0010)
#define GICC_IIDR	(GICC_BASE+0x00FC)
#define GICC_DIR	(GICC_BASE+0x1000)

#define GICV_CTLR (GICV_BASE+0x0)

#define GICD_CTL_ENABLE 0x1

/* Register bits */
#define GICD_TYPE_LINES 0x01F
#define GICD_TYPE_CPUS_SHIFT 5
#define GICD_TYPE_CPUS 0x0E0
#define GICD_TYPE_SEC 0x400

#define GICC_CTL_ENABLE 0x1
#define GICC_CTL_EOI (0x1 << 9)

#define GIC_PRI_IRQ 0xA0
#define GIC_PRI_IPI 0x90

/* GICD_SGIR defination */
#define GICD_SGIR_SGIINTID_SHIFT	0
#define GICD_SGIR_CPULIST_SHIFT		16
#define GICD_SGIR_LISTFILTER_SHIFT	24
#define GICD_SGIR_VAL(listfilter, cpulist, sgi)		\
	(((listfilter) << GICD_SGIR_LISTFILTER_SHIFT) |	\
	 ((cpulist) << GICD_SGIR_CPULIST_SHIFT) |	\
	 ((sgi) << GICD_SGIR_SGIINTID_SHIFT))

#define GIC_INTID_PPI                   16
#define GIC_INTID_SPI                   32
#define GIC_INTID_SPI_UART6             (GIC_INTID_SPI + 79)
#define GIC_INTID_PPI_EL1_PHYS_TIMER    (GIC_INTID_PPI + 14)
#define GIC_INTID_PPI_EL1_VIRT_TIMER    (GIC_INTID_PPI + 11)
#define GIC_INTID_PPI_NS_EL2_PHYS_TIMER (GIC_INTID_PPI + 10)
#define GIC_INTID_PPI_EL3_PHYS_TIMER    (GIC_INTID_PPI + 13)

#endif /* ARCH_AARCH64_PLAT_HIKEY970_MACHINE_H */