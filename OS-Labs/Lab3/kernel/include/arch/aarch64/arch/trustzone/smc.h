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

#ifndef ARCH_AARCH64_ARCH_TRUSTZONE_SMC_H
#define ARCH_AARCH64_ARCH_TRUSTZONE_SMC_H

#include <common/types.h>
#include <smc_num.h>

enum tz_switch_req {
        TZ_SWITCH_REQ_ENTRY_DONE,
        TZ_SWITCH_REQ_STD_REQUEST,
        TZ_SWITCH_REQ_STD_RESPONSE,
        TZ_SWITCH_REQ_NR
};

enum smc_ops_exit {
        SMC_OPS_NORMAL = 0x0,
        SMC_OPS_SCHEDTO = 0x1,
        SMC_OPS_START_SHADOW = 0x2,
        SMC_OPS_START_FIQSHD = 0x3,
        SMC_OPS_PROBE_ALIVE = 0x4,
        SMC_OPS_ABORT_TASK = 0x5,
        SMC_EXIT_NORMAL = 0x0,
        SMC_EXIT_PREEMPTED = 0x1,
        SMC_EXIT_SHADOW = 0x2,
        SMC_EXIT_ABORT = 0x3,
        SMC_EXIT_MAX = 0x4,
};

struct tz_vectors;

struct smc_registers {
        unsigned long x0;
        unsigned long x1;
        unsigned long x2;
        unsigned long x3;
        unsigned long x4;
};

extern struct tz_vectors tz_vectors;

typedef struct {
        unsigned long params_stack[8];
} __attribute__((packed)) kernel_shared_varibles_t;

void smc_init(void);

void smc_call(int fid, ...);

/* smc thread will call this function to wait for next smc request */
int ot_sys_tee_switch_req(struct smc_registers *regs_u);
/* idle thread will call this function to switch back to normal world */
int ot_sys_tee_wait_switch_req(struct smc_registers *regs_u);
/* gtask will call this function to get registers of the first smc request */
int ot_sys_tee_pull_kernel_var(kernel_shared_varibles_t *cmd_buf_addr_buf);

#endif /* ARCH_AARCH64_ARCH_TRUSTZONE_SMC_H */
