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

#ifndef COMMON_TEE_UUID_H
#define COMMON_TEE_UUID_H

#include <common/types.h>

#define NODE_LEN 8

typedef struct tee_uuid {
        u32 timeLow;
        u16 timeMid;
        u16 timeHiAndVersion;
        u8 clockSeqAndNode[NODE_LEN];
} TEE_UUID;

#endif /* COMMON_TEE_UUID_H */
