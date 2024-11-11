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

#ifndef USRSYSCALL_SMC_H
#define USRSYSCALL_SMC_H

#include <stddef.h>
#include <stdint.h>
#include <chcore/syscall.h>

struct cap_teesmc_buf {
        uint64_t ops;
        uint64_t ta;
        uint64_t target;
};

enum cap_teesmc_buf_ops {
        CAP_TEESMC_OPS_NORMAL = 0,
};

enum cap_teesmc_req {
        CAP_TEESMC_REQ_STARTTZ,
        CAP_TEESMC_REQ_IDLE,
};

int32_t smc_wait_switch_req(struct cap_teesmc_buf *buf);

int32_t smc_switch_req(enum cap_teesmc_req switch_req);

void init_teesmc_hdlr(void);

#endif
