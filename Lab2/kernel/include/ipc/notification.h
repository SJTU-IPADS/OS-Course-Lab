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

#ifndef IPC_NOTIFICATION_H
#define IPC_NOTIFICATION_H

#include <object/thread.h>
#include <object/object.h>

struct notification {
	unsigned int not_delivered_notifc_count;
	unsigned int waiting_threads_count;
	struct list_head waiting_threads;
	/*
	 * notifc_lock protects counter and list of waiting threads,
	 * including the internal states of waiting threads.
	 */
	struct lock notifc_lock;

	/* For recycling */
	int state;
};

struct irq_notification;

int init_notific(struct notification *notifc);
void deinit_notific(struct notification *notifc);
int wait_notific_internal(struct notification *notifc, bool is_block,
                          struct timespec *timeout, bool need_unlock,
                          bool need_obj_put);
int wait_notific(struct notification *notifc, bool is_block,
		struct timespec *timeout);
int signal_notific(struct notification *notifc);
int requeue_notific(struct notification *src_notifc, struct notification *dst_notifc);

void wait_irq_notific(struct irq_notification *notifc);
void signal_irq_notific(struct irq_notification *notifc);

void notification_deinit(void *ptr);

void try_remove_timeout(struct thread *target);

/* Syscalls */
cap_t sys_create_notifc(void);
int wait_notific_internal(struct notification *notifc, bool is_block,
                          struct timespec *timeout, bool need_unlock,
                          bool need_obj_put);
int sys_wait(cap_t notifc_cap, bool is_block, struct timespec *timeout);
int sys_notify(cap_t notifc_cap);

CAP_ALLOC_DECLARE(create_notific);

#endif /* IPC_NOTIFICATION_H */