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

#ifndef OBJECT_IRQ_H
#define OBJECT_IRQ_H

#include <ipc/notification.h>
#include <object/object.h>
#include <irq_num.h>

struct irq_notification {
	u32 intr_vector;
	u32 status;
	struct notification notifc;

	/*
	 * Debugging field: Using this field to avoid re-entry of
	 * a user-level interrupt handler thread.
	 */
	volatile u32 user_handler_ready;
};

extern struct irq_notification *irq_notifcs[MAX_IRQ_NUM];

int user_handle_irq(int irq);
void irq_deinit(void *irq_ptr);

/* Syscalls */
cap_t sys_irq_register(int irq);
int sys_irq_wait(cap_t irq_cap, bool is_block);
int sys_irq_ack(cap_t irq_cap);

#endif /* OBJECT_IRQ_H */