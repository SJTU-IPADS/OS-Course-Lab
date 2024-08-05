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

#ifndef IRQ_IPI_H
#define IRQ_IPI_H

#include <irq/irq.h>
#include <common/types.h>
#include <common/lock.h>
#include <arch/ipi.h>

void arch_send_ipi(u32 cpu, u32 ipi);

/* 7 u64 arg and 2 u32 (start/finish, vector) occupy one cacheline */
#define IPI_DATA_ARG_NUM (7)

void init_ipi_data(void);

/* IPI interfaces for achieving cross-core communication */

/* Sender side */
void prepare_ipi_tx(u32 target_cpu);
void set_ipi_tx_arg(u32 target_cpu, u32 arg_index, unsigned long val);
void start_ipi_tx(u32 target_cpu, u32 ipi_vector);
void wait_finish_ipi_tx(u32 target_cpu);
void send_ipi(u32 target_cpu, u32 ipi_vector);

/* Receiver side */
unsigned long get_ipi_tx_arg(u32 arg_index);
void handle_ipi(void);
void arch_handle_ipi(u32 ipi_vector);

#endif /* IRQ_IPI_H */