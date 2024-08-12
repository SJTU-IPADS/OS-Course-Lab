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

#ifndef CHCORE_PORT_CHCORE_SHM_H
#define CHCORE_PORT_CHCORE_SHM_H

#include <chcore/type.h>
#include <sys/shm.h>
#include <chcore/container/list.h>

struct local_shm_record {
        /* type is PMO_SHM */
        cap_t shm_cap;
        int shmid;
        unsigned long addr;
        struct list_head node;
};

int chcore_shmget(key_t key, size_t size, int flag);
void *chcore_shmat(int id, const void *addr, int flag);
int chcore_shmdt(const void *addr);
int chcore_shmctl(int id, int cmd, struct shmid_ds *buf);

#endif /* CHCORE_PORT_CHCORE_SHM_H */
