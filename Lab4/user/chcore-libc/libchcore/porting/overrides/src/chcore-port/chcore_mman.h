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

#ifndef CHCORE_PORT_CHCORE_MMAN_H
#define CHCORE_PORT_CHCORE_MMAN_H

#include <chcore/container/hashtable.h>
#include <chcore/type.h>
#include <chcore/defs.h>

#define MAP_SHARED  0x01
#define MAP_PRIVATE 0x02

#define PROT_NONE  0
#define PROT_READ  1
#define PROT_WRITE 2
#define PROT_EXEC  4

#define PROT_CHECK_MASK (~(PROT_NONE | PROT_READ | PROT_WRITE | PROT_EXEC))

#define HASH_TABLE_SIZE 509
#define VA_TO_KEY(va)   ((u32)((vaddr_t)va >> 12))

#define UNMAPSELF_STACK_SIZE (0x1000UL)

#if __SIZEOF_POINTER__ == 4
#define HEAP_START (0x60000000UL)
#elif defined(SV39)
#define HEAP_START (0x600000000UL)
#else
#define HEAP_START (0x600000000000UL)
#endif

struct pmo_node {
        cap_t cap;
        vaddr_t va;
        size_t pmo_size;
        ipc_struct_t *_fs_ipc_struct;
        struct list_head list_node;
        struct hlist_node hash_node;
};

void *chcore_mmap(void *start, size_t length, int prot, int flags, int fd,
                  off_t off);
void *chcore_fmap(void *start, size_t length, int prot, int flags, int fd,
                  off_t off);
int chcore_munmap(void *start, size_t length);
vaddr_t chcore_unmapself(void *start, size_t length);

#endif /* CHCORE_PORT_CHCORE_MMAN_H */
