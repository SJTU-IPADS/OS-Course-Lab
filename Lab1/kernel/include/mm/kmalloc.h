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

#ifndef MM_KMALLOC_H
#define MM_KMALLOC_H

#include <common/types.h>

void *_kmalloc(size_t size, bool is_record, size_t *real_size);
void *_get_pages(int order, bool is_record);
void _kfree(void *ptr, bool is_revoke_record);

void *kmalloc(unsigned long size);
void *kzalloc(unsigned long size);
void kfree(void* ptr);

/* Return vaddr of (1 << order) continous free physical pages */
void *get_pages(int order);
void free_pages(void* addr);
void get_mem_usage_msg(void);

void free_pages_without_record(void *addr);
void get_mem_usage_msg(void);

#endif /* MM_KMALLOC_H */
