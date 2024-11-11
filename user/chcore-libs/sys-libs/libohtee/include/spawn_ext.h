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

#ifndef SPAWN_EXT_H
#define SPAWN_EXT_H

#include <stddef.h>
#include <stdint.h>
#include <fcntl.h>
#include <pthread.h>
#include "tee_uuid.h"

#ifndef SPAWN_EXT_STRUCT
#define SPAWN_EXT_STRUCT

typedef int tid_t;

typedef struct spawn_ext {
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

int getuuid(pid_t pid, spawn_uuid_t *uuid);

int spawnattr_init(posix_spawnattr_t *attr);

void spawnattr_setuuid(posix_spawnattr_t *attr, const spawn_uuid_t *uuid);

int spawnattr_setheap(posix_spawnattr_t *attr, size_t size);

int spawnattr_setstack(posix_spawnattr_t *attr, size_t size);

int32_t thread_terminate(pthread_t thread);

int posix_spawn_ex(pid_t *pid, const char *path,
                   const posix_spawn_file_actions_t *file_action,
                   const posix_spawnattr_t *attrp, char **argv, char **envp,
                   int *tid);

size_t getstacksize(void);

#endif
