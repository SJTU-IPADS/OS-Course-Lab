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

#ifndef SYS_PRIORITIES_H
#define SYS_PRIORITIES_H

#include <stdint.h>

#define PRIO_TEE_MAX 56
#define PRIO_TEE_MIN 9

enum TEE_PRIO_offset {
        TEE_PRIO_SMCMGR_OFFSET,
        TEE_PRIO_GT_OFFSET,
        TEE_PRIO_DRV_OFFSET,
        TEE_PRIO_AGENT_OFFSET,
        TEE_PRIO_TA_OFFSET,
};

#define PRIO_TEE_SMCMGR (PRIO_TEE_MAX - TEE_PRIO_SMCMGR_OFFSET)
#define PRIO_TEE_GT     (PRIO_TEE_MAX - TEE_PRIO_GT_OFFSET)
#define PRIO_TEE_DRV    (PRIO_TEE_MAX - TEE_PRIO_DRV_OFFSET)
#define PRIO_TEE_AGENT  (PRIO_TEE_MAX - TEE_PRIO_AGENT_OFFSET)
#define PRIO_TEE_TA     (PRIO_TEE_MAX - TEE_PRIO_TA_OFFSET)

#define PRIO_TEE_SMCMGR_IDLE 1

int32_t set_priority(unsigned int prio);

#endif
