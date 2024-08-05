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

#ifndef ARCH_AARCH64_ARCH_SCHED_ARCH_SCHED_H
#define ARCH_AARCH64_ARCH_SCHED_ARCH_SCHED_H

#include <arch/machine/registers.h>

/* size in registers.h (to be used in asm) */
typedef struct arch_exec_context {
        unsigned long reg[REG_NUM];
} arch_exec_ctx_t;

#define TLS_REG_NUM 1

#endif /* ARCH_AARCH64_ARCH_SCHED_ARCH_SCHED_H */
