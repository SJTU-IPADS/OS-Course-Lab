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

#ifndef OPENTRUSTEE_DEFS_H
#define OPENTRUSTEE_DEFS_H

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

#endif

#ifndef SPAWN_EXT_STRUCT
#define SPAWN_EXT_STRUCT

typedef struct spawn_ext
{
        uint64_t uuid_valid;
        TEE_UUID uuid;
} spawn_uuid_t;

typedef struct {
        unsigned version;
        uint64_t stack_size;
        uint64_t heap_size;
        unsigned int flags;
        spawn_uuid_t uuid;
        int32_t ptid;
} posix_spawnattr_t;

typedef struct {
	int __pad0[2];
	void *__actions;
	int __pad[16];
} posix_spawn_file_actions_t;

#endif

#endif /* OPENTRUSTEE_DEFS_H */
