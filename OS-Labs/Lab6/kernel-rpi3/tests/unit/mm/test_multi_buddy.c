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

#include <stdlib.h>
#include <sys/mman.h>

/* unit test */
#include "minunit.h"

#include <mm/kmalloc.h>
#include <mm/slab.h>
#include <mm/buddy.h>

#define NPAGES          (128 * 1024)
#define PAGE_SIZE       0x1000
#define BUDDY_MAX_ORDER 13

struct phys_mem_pool global_mem[N_PHYS_MEM_POOLS];

int get_chunk_num(int index, u64 order)
{
        struct free_list *list;
        struct phys_mem_pool *pool = &global_mem[index];

        list = pool->free_lists + order;
        return list->nr_free;
}

MU_TEST(test_multi_buddy)
{
        void *start;
        unsigned long npages;
        unsigned long size;
        unsigned long start_addr;
        int i;
        int cnt;
        int cnt1, cnt2;
        char *buf1[100];
        char *buf2[100];

        /* free_mem_size: npages * 0x1000 */
        npages = NPAGES;
        /* allocate the page_metadata area */
        size = npages * sizeof(struct page);
        start = mmap((void *)0x50000000000,
                     size,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS,
                     -1,
                     0);
        /* allocate the physical memory area */
        size = npages * (0x1000);
        start_addr = (u64)mmap((void *)0x60000000000,
                               size,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED | MAP_ANONYMOUS,
                               -1,
                               0);
        init_buddy(&global_mem[0], start, start_addr, npages);

        /* free_mem_size: npages * 0x1000 */
        npages = NPAGES;
        /* allocate the page_metadata area */
        size = npages * sizeof(struct page);
        start = mmap((void *)0x50100000000,
                     size,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS,
                     -1,
                     0);
        /* allocate the physical memory area */
        size = npages * (0x1000);
        start_addr = (u64)mmap((void *)0x60100000000,
                               size,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED | MAP_ANONYMOUS,
                               -1,
                               0);
        init_buddy(&global_mem[1], start, start_addr, npages);

        /* Request all the chunks of the max order */
        cnt1 = get_chunk_num(0, BUDDY_MAX_ORDER);
        for (i = 0; i < cnt1; ++i) {
                buf1[i] = kmalloc(PAGE_SIZE << BUDDY_MAX_ORDER);
                mu_check(buf1[i] != NULL);
        }
        mu_check(get_chunk_num(0, BUDDY_MAX_ORDER) == 0);

        // /* Request all the chunks of the max order */
        cnt2 = get_chunk_num(1, BUDDY_MAX_ORDER);
        for (i = 0; i < cnt2; ++i) {
                buf2[i] = kmalloc(PAGE_SIZE << BUDDY_MAX_ORDER);
                mu_check(buf2[i] != NULL);
        }
        mu_check(get_chunk_num(1, BUDDY_MAX_ORDER) == 0);

        for (i = 0; i < cnt1; ++i) {
                kfree(buf1[i]);
                mu_check(get_chunk_num(0, BUDDY_MAX_ORDER) == i + 1);
        }

        for (i = 0; i < cnt2; ++i) {
                kfree(buf2[i]);
                mu_check(get_chunk_num(1, BUDDY_MAX_ORDER) == i + 1);
        }
}

MU_TEST_SUITE(test_suite)
{
        MU_RUN_TEST(test_multi_buddy);
}

int main(int argc, char *argv[])
{
        MU_RUN_SUITE(test_suite);
        MU_REPORT();
        return minunit_status;
}
