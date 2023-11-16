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

#ifndef ARCH_AARCH64_PLAT_RASPI3_MACHINE_H
#define ARCH_AARCH64_PLAT_RASPI3_MACHINE_H

#include <common/vars.h>

/* raspi3 config */
#define PLAT_CPU_NUM    4
#define PLAT_RASPI3

#define BCM2835_REG(x)           (*(volatile uint32_t *)(x))
#define BCM2835_BIT(n)           (1 << (n))

// Peripheral Base Register Address
#define RPI_PERIPHERAL_BASE      (KBASE+0x3F000000)
#define RPI_PERIPHERAL_SIZE      (KBASE+0x01000000)

// Internal ARM Timer Registers
#define BCM2835_CLOCK_FREQ       250000000
#define BCM2835_TIMER_BASE       (RPI_PERIPHERAL_BASE + 0xB400)
#define BCM2835_TIMER_LOD        (BCM2835_TIMER_BASE+0x00)
#define BCM2835_TIMER_VAL        (BCM2835_TIMER_BASE+0x04)
#define BCM2835_TIMER_CTL        (BCM2835_TIMER_BASE+0x08)
#define BCM2835_TIMER_CLI        (BCM2835_TIMER_BASE+0x0C)
#define BCM2835_TIMER_RIS        (BCM2835_TIMER_BASE+0x10)
#define BCM2835_TIMER_MIS        (BCM2835_TIMER_BASE+0x14)
#define BCM2835_TIMER_RLD        (BCM2835_TIMER_BASE+0x18)
#define BCM2835_TIMER_DIV        (BCM2835_TIMER_BASE+0x1C)
#define BCM2835_TIMER_CNT        (BCM2835_TIMER_BASE+0x20)
#define BCM2835_TIMER_PRESCALE    0xF9

// GPIO Registers
#define BCM2835_GPIO_REGS_BASE   (RPI_PERIPHERAL_BASE + 0x200000)
#define BCM2835_GPIO_GPFSEL1     (BCM2835_GPIO_REGS_BASE+0x04)
#define BCM2835_GPIO_GPSET0      (BCM2835_GPIO_REGS_BASE+0x1C)
#define BCM2835_GPIO_GPCLR0      (BCM2835_GPIO_REGS_BASE+0x28)
#define BCM2835_GPIO_GPPUD       (BCM2835_GPIO_REGS_BASE+0x94)
#define BCM2835_GPIO_GPPUDCLK0   (BCM2835_GPIO_REGS_BASE+0x98)

// AUX Registers
#define BCM2835_AUX_BASE         (RPI_PERIPHERAL_BASE + 0x215000)
#ifndef AUX_ENABLES  /* May be also defined in peripherals.h*/
#define AUX_ENABLES              (BCM2835_AUX_BASE+0x04)
#define AUX_MU_IO_REG            (BCM2835_AUX_BASE+0x40)
#define AUX_MU_IER_REG           (BCM2835_AUX_BASE+0x44)
#define AUX_MU_IIR_REG           (BCM2835_AUX_BASE+0x48)
#define AUX_MU_LCR_REG           (BCM2835_AUX_BASE+0x4C)
#define AUX_MU_MCR_REG           (BCM2835_AUX_BASE+0x50)
#define AUX_MU_LSR_REG           (BCM2835_AUX_BASE+0x54)
#define AUX_MU_MSR_REG           (BCM2835_AUX_BASE+0x58)
#define AUX_MU_SCRATCH           (BCM2835_AUX_BASE+0x5C)
#define AUX_MU_CNTL_REG          (BCM2835_AUX_BASE+0x60)
#define AUX_MU_STAT_REG          (BCM2835_AUX_BASE+0x64)
#define AUX_MU_BAUD_REG          (BCM2835_AUX_BASE+0x68)
#endif

// UART 0 (PL011) Registers
#define BCM2835_UART0_BASE       (RPI_PERIPHERAL_BASE + 0x201000)
#define BCM2835_UART0_DR         (BCM2835_UART0_BASE+0x00)
#define BCM2835_UART0_RSRECR     (BCM2835_UART0_BASE+0x04)
#define BCM2835_UART0_FR         (BCM2835_UART0_BASE+0x18)
#define BCM2835_UART0_ILPR       (BCM2835_UART0_BASE+0x20)
#define BCM2835_UART0_IBRD       (BCM2835_UART0_BASE+0x24)
#define BCM2835_UART0_FBRD       (BCM2835_UART0_BASE+0x28)
#define BCM2835_UART0_LCRH       (BCM2835_UART0_BASE+0x2C)
#define BCM2835_UART0_CR         (BCM2835_UART0_BASE+0x30)
#define BCM2835_UART0_IFLS       (BCM2835_UART0_BASE+0x34)
#define BCM2835_UART0_IMSC       (BCM2835_UART0_BASE+0x38)
#define BCM2835_UART0_RIS        (BCM2835_UART0_BASE+0x3C)
#define BCM2835_UART0_MIS        (BCM2835_UART0_BASE+0x40)
#define BCM2835_UART0_ICR        (BCM2835_UART0_BASE+0x44)
#define BCM2835_UART0_DMACR      (BCM2835_UART0_BASE+0x48)
#define BCM2835_UART0_ITCR       (BCM2835_UART0_BASE+0x80)
#define BCM2835_UART0_ITIP       (BCM2835_UART0_BASE+0x84)
#define BCM2835_UART0_ITOP       (BCM2835_UART0_BASE+0x88)
#define BCM2835_UART0_TDR        (BCM2835_UART0_BASE+0x8C)

#define BCM2835_UART0_MIS_RX    0x10
#define BCM2835_UART0_MIS_TX    0x20
#define BCM2835_UART0_IMSC_RX   0x10
#define BCM2835_UART0_IMSC_TX   0x20
#define BCM2835_UART0_FR_RXFE   0x10
#define BCM2835_UART0_FR_TXFF   0x20
#define BCM2835_UART0_ICR_RX    0x10
#define BCM2835_UART0_ICR_TX    0x20

// IRQ Registers
#define BCM2835_BASE_INTC         (RPI_PERIPHERAL_BASE + 0xB200)
#define BCM2835_IRQ_BASIC         (BCM2835_BASE_INTC + 0x00)
#define BCM2835_IRQ_PENDING1      (BCM2835_BASE_INTC + 0x04)
#define BCM2835_IRQ_PENDING2      (BCM2835_BASE_INTC + 0x08)
#define BCM2835_IRQ_FIQ_CTRL      (BCM2835_BASE_INTC + 0x0C)
#define BCM2835_IRQ_ENABLE1       (BCM2835_BASE_INTC + 0x10)
#define BCM2835_IRQ_ENABLE2       (BCM2835_BASE_INTC + 0x14)
#define BCM2835_IRQ_ENABLE_BASIC  (BCM2835_BASE_INTC + 0x18)
#define BCM2835_IRQ_DISABLE1      (BCM2835_BASE_INTC + 0x1C)
#define BCM2835_IRQ_DISABLE2      (BCM2835_BASE_INTC + 0x20)
#define BCM2835_IRQ_DISABLE_BASIC (BCM2835_BASE_INTC + 0x24)

#define SYSTEM_TIMER_IRQ_0	(1 << 0)
#define SYSTEM_TIMER_IRQ_1	(1 << 1)
#define SYSTEM_TIMER_IRQ_2	(1 << 2)
#define SYSTEM_TIMER_IRQ_3	(1 << 3)

// GPU Timer Registers
#define BCM2835_GPU_TIMER_BASE    (RPI_PERIPHERAL_BASE + 0x3000)
#define BCM2835_GPU_TIMER_CS      (BCM2835_TIMER_BASE+0x00)
#define BCM2835_GPU_TIMER_CLO     (BCM2835_TIMER_BASE+0x04)
#define BCM2835_GPU_TIMER_CHI     (BCM2835_TIMER_BASE+0x08)
#define BCM2835_GPU_TIMER_C0      (BCM2835_TIMER_BASE+0x0C)
#define BCM2835_GPU_TIMER_C1      (BCM2835_TIMER_BASE+0x10)
#define BCM2835_GPU_TIMER_C2      (BCM2835_TIMER_BASE+0x14)
#define BCM2835_GPU_TIMER_C3      (BCM2835_TIMER_BASE+0x18)

// raspberrypi_interrupt Interrrupt Support
#define BCM2835_INTC_TOTAL_IRQ       64 + 8
#define BCM2835_IRQ_ID_AUX           29
#define BCM2835_IRQ_ID_SPI_SLAVE     43
#define BCM2835_IRQ_ID_PWA0          45
#define BCM2835_IRQ_ID_PWA1          46
#define BCM2835_IRQ_ID_SMI           48
#define BCM2835_IRQ_ID_GPIO_0        49
#define BCM2835_IRQ_ID_GPIO_1        50
#define BCM2835_IRQ_ID_GPIO_2        51
#define BCM2835_IRQ_ID_GPIO_3        52
#define BCM2835_IRQ_ID_I2C           53
#define BCM2835_IRQ_ID_SPI           54
#define BCM2835_IRQ_ID_PCM           55
#define BCM2835_IRQ_ID_UART          57

#define BCM2835_IRQ_ID_BASIC_BASE_ID 64
#define BCM2835_IRQ_ID_TIMER_0       64
#define BCM2835_IRQ_ID_MAILBOX_0     65
#define BCM2835_IRQ_ID_DOORBELL_0    66
#define BCM2835_IRQ_ID_DOORBELL_1    67
#define BCM2835_IRQ_ID_GPU0_HALTED   68
#define BCM2835_IRQ_ID_GPU1_HALTED   69
#define BCM2835_IRQ_ID_ILL_ACCESS_1  70
#define BCM2835_IRQ_ID_ILL_ACCESS_0  71

#define INTERRUPT_VECTOR_MIN    (0)
#define INTERRUPT_VECTOR_MAX    (BCM2835_INTC_TOTAL_IRQ - 1)

#define IRQ_COUNT               (BCM2835_INTC_TOTAL_IRQ)

// Where to route timer interrupt to, IRQ/FIQ
// Setting both the IRQ and FIQ bit gives an FIQ
#define TIMER0_IRQ 0x01
#define TIMER1_IRQ 0x02
#define TIMER2_IRQ 0x04
#define TIMER3_IRQ 0x08

#define TIMER0_FIQ 0x10
#define TIMER1_FIQ 0x20
#define TIMER2_FIQ 0x40
#define TIMER3_FIQ 0x80

// IRQ & FIQ source registers
#define CORE_IRQ_BASE                   (KBASE + 0x40000060)
#define CORE0_IRQ_SOURCE                (CORE_IRQ_BASE + 0x0)
#define CORE1_IRQ_SOURCE                (CORE_IRQ_BASE + 0x4)
#define CORE2_IRQ_SOURCE                (CORE_IRQ_BASE + 0x8)
#define CORE3_IRQ_SOURCE                (CORE_IRQ_BASE + 0xc)

#define CORE_FIQ_BASE                   (KBASE + 0x40000070)
#define CORE0_FIQ_SOURCE                (CORE_IRQ_BASE + 0x0)
#define CORE1_FIQ_SOURCE                (CORE_IRQ_BASE + 0x4)
#define CORE2_FIQ_SOURCE                (CORE_IRQ_BASE + 0x8)
#define CORE3_FIQ_SOURCE                (CORE_IRQ_BASE + 0xc)

// Mailbox registers
#define CORE_MBOX_CTL_BASE		(KBASE + 0x40000050)
#define CORE_MBOX_CTL(cpu)		(CORE_MBOX_CTL_BASE + cpu * 0x4)

#define CORE_MBOX_SET_BASE		(KBASE + 0x40000080)
#define CORE_MBOX_SET(cpu, box)		(CORE_MBOX_SET_BASE + cpu * 0x10 + \
					 box * 0x4)

#define CORE_MBOX_RDCLR_BASE		(KBASE + 0x400000c0)
#define CORE_MBOX_RDCLR(cpu, box)	(CORE_MBOX_RDCLR_BASE + cpu * 0x10 + \
					 box * 0x4)

// FIXME:
// 1. Make the definitions precise according to Section-4.6 and 4.10 in QA7.pdf.
// 2. Move the timer init code into user space and remove useless code

// TODO: INT_SRC_TIMER1 should not be used as the arg to enable timer
// interrupt source bits


/*
 * According to Section-4.10 in Quad-A7 control (QA7.pdf):
 * The cores can get an interrupt or fast interrupt from many places.
 * In order to speed up interrupt processing the interrupt source registers
 * shows what the source bits are for the IRQ/FIQ.
 * As is usual there is a register for each processor.
 *
 * There are four interrupt source registers.
 * Address: 0x4000_0060 Core0 interrupt source
 * Address: 0x4000_0064 Core1 interrupt source
 * Address: 0x4000_0068 Core2 interrupt source
 * Address: 0x4000_006C Core3 interrupt source
 */
#define INT_SRC_TIMER0			0x001 	// CNTPSIRQ
#define INT_SRC_TIMER1			0x002   // CNTPNSIRQ
#define INT_SRC_TIMER2			0x004   // CNTHPIRQ
#define INT_SRC_TIMER3			0x008   // CNTVIRQ
#define INT_SRC_MBOX0			0x010
#define INT_SRC_MBOX1			0x020
#define INT_SRC_MBOX2			0x040
#define INT_SRC_MBOX3			0x080

/*
 * According to Section-4.6 in Quad-A7 control (QA7.pdf):
 * There are four core timer control registers.
 * The registers allow you to enable or disable an IRQ or FIQ interrupt.
 */
#define CORE_TIMER_IRQCNTL_BASE         (KBASE + 0x40000040)
#define CORE0_TIMER_IRQCNTL             (CORE_TIMER_IRQCNTL_BASE + 0x0)
#define CORE1_TIMER_IRQCNTL             (CORE_TIMER_IRQCNTL_BASE + 0x4)
#define CORE2_TIMER_IRQCNTL             (CORE_TIMER_IRQCNTL_BASE + 0x8)
#define CORE3_TIMER_IRQCNTL             (CORE_TIMER_IRQCNTL_BASE + 0xc)
/* physical and non-secure timer control*/
#define CNTPNSIRQ 0x2 // bit 1

#endif /* ARCH_AARCH64_PLAT_RASPI3_MACHINE_H */