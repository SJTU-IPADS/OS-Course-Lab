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

#ifndef IRQ_IRQ_H
#define IRQ_IRQ_H

#include <object/thread.h>
#include <irq_num.h>

#define HANDLE_KERNEL	0
#define HANDLE_USER	1
extern unsigned char irq_handle_type[MAX_IRQ_NUM];

void arch_interrupt_init(void); /* in arch/xxx/irq/irq_entry.c */
void arch_interrupt_init_per_cpu(void); /* in arch/xxx/irq/irq_entry.c */
void arch_enable_irqno(int irqno);
void arch_disable_irqno(int irqno);
void arch_enable_irq(void);
void arch_disable_irq(void);

/* XXX: only needed in ARM and SPARC */
void plat_handle_irq(void);     /* in arch/xxx/plat/xxx/irq/irq.c */
void plat_handle_irqno(int irq);     /* in arch/xxx/plat/xxx/irq/irq.c */
void plat_interrupt_init(void); /* in arch/xxx/plat/xxx/irq/irq.c */
void plat_disable_timer(void);
void plat_enable_timer(void);
void plat_enable_irqno(int irq);
void plat_disable_irqno(int irq);
void plat_ack_irq(int irq);
int plat_get_irq_info(void *user_buf, unsigned long size);

/* 
 * Syscalls
 * The following syscalls are only needed on SPRAC for VxWorks.
 */
void sys_configure_irq(unsigned long all_irq, unsigned long irqno, unsigned long enable);

#endif /* IRQ_IRQ_H */
