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

#define IRQ_LEVEL_REG	0xf0000200
#define IRQ_PENDING_REG	0xf0000204
#define IRQ_CLEAR_REG	0xf000020c
#define IRQ_STATUS_REG	0xf0000210

#define IRQ_MASK_REG	0xf0000240
#define IRQ_FORCE_REG	0xf0000280
#define IRQ_EXETEND_REG	0xf00002C0

#define IRQ_UART1	(2)
#define IRQ_UART2	(3)
#define IRQ_LATCH	(4)
#define IRQ_GRETH	(5)
#define IRQ_GPIO10	(10)
#define IRQ_EXETEND	    (11)
#define IRQ_IPI         (12)
#define IRQ_GPIO13       (13)
#define IRQ_GPIO14       (14)
#define IRQ_GPIO15       (15)

#define TIMER1_IRQ	(1 << 6)
#define TIMER2_IRQ	(1 << 7)
#define TIMER3_IRQ	(1 << 8)
#define TIMER4_IRQ	(1 << 9)

#define IRQ_STATUS_REG_NCPU_SHIFT	(28)
#define IRQ_STATUS_REG_NCPU_MASK	(0xff)

extern int timer_irq_start;

void plat_handle_global_timer_irq(void);
