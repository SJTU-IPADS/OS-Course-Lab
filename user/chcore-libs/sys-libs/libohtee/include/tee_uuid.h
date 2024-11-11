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

#ifndef TEE_UUID_H
#define TEE_UUID_H

#include <stdint.h>

#ifndef TEE_UUID_STRUCT
#define TEE_UUID_STRUCT

#define NODE_LEN 8

typedef struct tee_uuid {
        uint32_t timeLow;
        uint16_t timeMid;
        uint16_t timeHiAndVersion;
        uint8_t clockSeqAndNode[NODE_LEN];
} TEE_UUID;

#endif /* TEE_UUID_STRUCT */

#endif /* TEE_UUID_H */
