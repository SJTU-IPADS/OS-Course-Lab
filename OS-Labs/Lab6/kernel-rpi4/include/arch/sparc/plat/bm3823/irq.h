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

#pragma once

/*
 * Note that all these APB registers are accessed
 * through virtual addresses, the physical addresses
 * of these registers are 0x8xxxxxxx.
 */

#define IRQ_STATUS_REG	0xf0000010

/* The defination of FORCE_REG and MASK_REG are not identical */
#define IRQ_MASK_REG	0xf0000090
#define IRQ_FORCE_REG	0xf0000094
#define IRQ_CLEAR_REG   0xf000009c
#define IRQ_EXCONF_REQ  0xf00003c0

#define IRQ_EXTERNAL1   (4)
#define IRQ_EXTERNAL2   (5)
#define IRQ_EXTERNAL6   (9)

#define IRQ_UART1	(3)
#define IRQ_IPI 	(9)

#define TIMER1_IRQ	(1 << 6)
#define TIMER2_IRQ	(1 << 7)
#define TIMER3_IRQ	(1 << 8)
#define TIMER4_IRQ	(1 << 9)

#define IRQ_STATUS_REG_NCPU_SHIFT	(28)
#define IRQ_STATUS_REG_NCPU_MASK	(0xff)

extern int timer_irq_start;

void plat_handle_global_timer_irq(void);
