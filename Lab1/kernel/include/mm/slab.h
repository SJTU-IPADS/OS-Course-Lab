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

#ifndef MM_SLAB_H
#define MM_SLAB_H

#include <common/list.h>

/*
 * order range: [SLAB_MIN_ORDER, SLAB_MAX_ORDER]
 * ChCore prepares the slab for each order in the range.
 */
#define SLAB_MIN_ORDER (5)
#define SLAB_MAX_ORDER (11)

/* The size of one slab is 128K. */
#define SIZE_OF_ONE_SLAB (128*1024)

/* slab_header resides in the beginning of each slab (i.e., occupies the first slot). */
struct slab_header {
        /* The list of free slots, which can be converted to struct slab_slot_list. */
        void *free_list_head;
        /* Partial slab list. */
        struct list_head node;

        int order;
        unsigned short total_free_cnt; /* MAX: 65536 */
        unsigned short current_free_cnt;
};

/* Each free slot in one slab is regarded as slab_slot_list. */
struct slab_slot_list {
        void *next_free;
};

struct slab_pointer {
        struct slab_header *current_slab;
        struct list_head partial_slab_list;
};

/* All interfaces are kernel/mm module internal interfaces. */
void init_slab(void);
void *alloc_in_slab(unsigned long, size_t *);
void free_in_slab(void *addr);
unsigned long get_free_mem_size_from_slab(void);

#endif /* MM_SLAB_H */
