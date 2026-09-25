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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <sys/mman.h>
#include <string.h>

/* unit test */
#include "minunit.h"

#include <mm/slab.h>
#include <mm/buddy.h>
#include <mm/kmalloc.h>

#define NPAGES (512 * 1024)

struct phys_mem_pool global_mem[N_PHYS_MEM_POOLS];

/* test buddy allocator */
static unsigned long buddy_num_free_page(struct phys_mem_pool *zone)
{
        unsigned long i, ret;

        ret = 0;
        for (i = 0; i < BUDDY_MAX_ORDER; ++i) {
                ret += zone->free_lists[i].nr_free;
        }
        return ret;
}

static void valid_page_idx(struct phys_mem_pool *zone, long idx)
{
        mu_assert((idx < zone->pool_phys_page_num) && (idx >= 0),
                  "invalid page idx");
}

static unsigned long get_page_idx(struct phys_mem_pool *zone, struct page *page)
{
        long idx;

        idx = page - zone->page_metadata;
        valid_page_idx(zone, idx);

        return idx;
}

static void test_alloc(struct phys_mem_pool *zone, long n, long order)
{
        long i;
        struct page *page;

        for (i = 0; i < n; ++i) {
                page = buddy_get_pages(zone, order);
                mu_check(page != NULL);
                get_page_idx(zone, page);
        }
        return;
}

void test_buddy(void)
{
        void *start;
        unsigned long npages;
        unsigned long size;
        unsigned long start_addr;

        long nget;
        long ncheck;

        struct page *page;
        long i;

        /* free_mem_size: npages * 0x1000 */
        npages = NPAGES;

        /* prepare the page metadata area */
        size = npages * sizeof(struct page);
        start = mmap((void *)0x50000000000,
                     size,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS,
                     -1,
                     0);

        /* prepare the physical memory area */
        size = npages * (0x1000);
        start_addr = (u64)mmap((void *)0x60000000000,
                               size,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED | MAP_ANONYMOUS,
                               -1,
                               0);

        init_buddy(&global_mem[0], start, start_addr, npages);

        /* check the init state */
        nget = buddy_num_free_page(&global_mem[0]);
        ncheck = npages / powl(2, BUDDY_MAX_ORDER - 1);
        mu_check(nget == ncheck);

        /* alloc single page for $npages times */
        test_alloc(&global_mem[0], npages, 0);

        /* should have 0 free pages */
        nget = buddy_num_free_page(&global_mem[0]);
        ncheck = 0;
        mu_check(nget == ncheck);

        /* free all pages */
        for (i = 0; i < npages; ++i) {
                page = global_mem[0].page_metadata + i;
                buddy_free_pages(&global_mem[0], page);
        }
        nget = buddy_num_free_page(&global_mem[0]);
        ncheck = npages / powl(2, BUDDY_MAX_ORDER - 1);
        mu_check(nget == ncheck);

        /* alloc 2-pages for $npages/2 times */
        test_alloc(&global_mem[0], npages / 2, 1);

        /* should have 0 free pages */
        nget = buddy_num_free_page(&global_mem[0]);
        ncheck = 0;
        mu_check(nget == ncheck);

        /* free all pages */
        for (i = 0; i < npages; i += 2) {
                page = global_mem[0].page_metadata + i;
                buddy_free_pages(&global_mem[0], page);
        }
        nget = buddy_num_free_page(&global_mem[0]);
        ncheck = npages / powl(2, BUDDY_MAX_ORDER - 1);
        mu_check(nget == ncheck);

        /* alloc MAX_ORDER-pages for  */
        test_alloc(&global_mem[0],
                   npages / powl(2, BUDDY_MAX_ORDER - 1),
                   BUDDY_MAX_ORDER - 1);

        /* should have 0 free pages */
        nget = buddy_num_free_page(&global_mem[0]);
        ncheck = 0;
        mu_check(nget == ncheck);

        /* free all pages */
        for (i = 0; i < npages; i += powl(2, BUDDY_MAX_ORDER - 1)) {
                page = global_mem[0].page_metadata + i;
                buddy_free_pages(&global_mem[0], page);
        }
        nget = buddy_num_free_page(&global_mem[0]);
        ncheck = npages / powl(2, BUDDY_MAX_ORDER - 1);
        mu_check(nget == ncheck);

        /* alloc single page for $npages times */
        test_alloc(&global_mem[0], npages, 0);

        /* should have 0 free pages */
        nget = buddy_num_free_page(&global_mem[0]);
        ncheck = 0;
        mu_check(nget == ncheck);

        /* free a half pages */
        for (i = 0; i < npages; i += 2) {
                page = global_mem[0].page_metadata + i;
                buddy_free_pages(&global_mem[0], page);
        }
        nget = buddy_num_free_page(&global_mem[0]);
        ncheck = npages / 2;
        mu_check(nget == ncheck);

        /* free another half pages */
        for (i = 1; i < npages; i += 2) {
                page = global_mem[0].page_metadata + i;
                buddy_free_pages(&global_mem[0], page);
        }
        nget = buddy_num_free_page(&global_mem[0]);
        ncheck = npages / powl(2, BUDDY_MAX_ORDER - 1);
        mu_check(nget == ncheck);
}

/* test slab allocator */

extern struct slab_pointer slab_pool[];

static void check_slab_range(struct slab_header *slab, void *addr, int cnt)
{
        if (slab == NULL)
                return;

        if (!((addr > (void *)slab)
              && (addr < (void *)((unsigned long)slab + SIZE_OF_ONE_SLAB)))) {
                printf("\n%s fail: cnt: %d, slab addr: 0x%lx, slot addr: 0x%lx\n",
                       __func__,
                       cnt,
                       (unsigned long)slab,
                       (unsigned long)addr);
                exit(-1);
        }
        mu_check((void *)addr > (void *)slab);
        mu_check((void *)addr < ((void *)((unsigned long)slab + SIZE_OF_ONE_SLAB)));
}

static void dump_slab(int order, int cnt_check)
{
        int cnt;

        extern unsigned long get_free_slot_number(int);
        cnt = get_free_slot_number(order);

        printf("cnt: %d, cnt_check: %d\n", cnt, cnt_check);
        if (cnt != cnt_check) {
                exit(1);
        }
        mu_check(cnt == cnt_check);
}

void print_slab(int order)
{
        printf("slab_pool[%d] current_slab: %p\n",
               order,
               slab_pool[order].current_slab);
        if (slab_pool[order].current_slab)
                printf("current_slab->current_free_cnt: %d\n",
                       slab_pool[order].current_slab->current_free_cnt);
}

void test_slab(void)
{
        int i;
        int j;

        int cnt;
        int check_cnt;
        void **ptr;

        printf("func: %s, line: %d\n", __func__, __LINE__);
        init_slab();

        /* check init state */
        for (i = SLAB_MIN_ORDER; i <= SLAB_MAX_ORDER; ++i) {
                dump_slab(i, 0);
        }

        printf("init pass: func: %s, line: %d\n", __func__, __LINE__);

        /* check slab alloc/free */
        ptr = malloc(SIZE_OF_ONE_SLAB / powl(2, SLAB_MIN_ORDER)
                     * sizeof(void *));
        mu_check(ptr != NULL);

        for (i = SLAB_MIN_ORDER; i <= SLAB_MAX_ORDER; ++i) {
                printf("test: slab %d alloc/free\n", i);
                cnt = SIZE_OF_ONE_SLAB / powl(2, i) - 1;
                for (j = 0; j < cnt; ++j) {
                        ptr[j] = alloc_in_slab(powl(2, i), NULL);
                        mu_check(ptr[j] != NULL);
                        check_slab_range(slab_pool[i].current_slab, ptr[j], j);
                }
                printf("test: slab %d alloc/free first dump\n", i);
                print_slab(i);
                dump_slab(i, 0);

                j = 0;
                free_in_slab(ptr[j]);
                print_slab(i);
                dump_slab(i, 1);

                for (j = 1; j < cnt - 1; ++j) {
                        free_in_slab(ptr[j]);
                }
                print_slab(i);
                dump_slab(i, cnt - 1);

                free_in_slab(ptr[j]);
                print_slab(i);
                dump_slab(i, 0);
                printf("test: slab %d alloc/free done\n", i);
        }

        printf("test: trigger new slab allocation\n");
        /* trigger new slab allocations */
        for (i = SLAB_MIN_ORDER; i <= SLAB_MAX_ORDER; ++i) {
                /* needs two slabs */
                cnt = SIZE_OF_ONE_SLAB / powl(2, i);
                for (j = 0; j < cnt; ++j) {
                        ptr[j] = alloc_in_slab(powl(2, i), NULL);
                        mu_check(ptr[j] != NULL);
                        check_slab_range(slab_pool[i].current_slab, ptr[j], j);
                }
                dump_slab(i, cnt - 2);

                for (j = 0; j < cnt; ++j) {
                        free_in_slab(ptr[j]);
                }
                dump_slab(i, 0);
        }
        free(ptr);

        /* trigger a third slab allocation */
        for (i = SLAB_MIN_ORDER; i <= SLAB_MAX_ORDER; ++i) {
                /* needs three slabs */
                cnt = SIZE_OF_ONE_SLAB / powl(2, i) - 1;
                check_cnt = cnt - 1;
                cnt = (cnt * 2 + 1);
                ptr = malloc(cnt * sizeof(void *));
                for (j = 0; j < cnt; ++j) {
                        ptr[j] = alloc_in_slab(powl(2, i), NULL);
                        mu_check(ptr[j] != NULL);
                        check_slab_range(slab_pool[i].current_slab, ptr[j], j);
                }

                dump_slab(i, check_cnt);

                for (j = 0; j < cnt; ++j) {
                        free_in_slab(ptr[j]);
                }
                dump_slab(i, 0);
                free(ptr);
        }

        /* random alloc and free */
        for (i = SLAB_MIN_ORDER; i <= SLAB_MAX_ORDER; ++i) {
                cnt = 1024 * 512;
                ptr = malloc(cnt * sizeof(void *));
                for (j = 0; j < cnt; ++j) {
                        ptr[j] = alloc_in_slab(powl(2, i), NULL);
                        mu_check(ptr[j] != NULL);
                        *(int *)ptr[j] = j;
                        check_slab_range(slab_pool[i].current_slab, ptr[j], j);
                }

                for (j = 0; j < cnt; j += 2) {
                        mu_check(*(int *)ptr[j] == j);
                        free_in_slab(ptr[j]);
                }

                for (j = 1; j < cnt; j += 2) {
                        mu_check(*(int *)ptr[j] == j);
                        free_in_slab(ptr[j]);
                }

                cnt /= 2;

                for (j = 0; j < cnt; ++j) {
                        ptr[j] = alloc_in_slab(powl(2, i), NULL);
                        mu_check(ptr[j] != NULL);
                        *(int *)ptr[j] = j;
                        check_slab_range(slab_pool[i].current_slab, ptr[j], j);
                }

                for (j = 0; j < cnt; j += 3) {
                        mu_check(*(int *)ptr[j] == j);
                        free_in_slab(ptr[j]);
                }

                for (j = 0; j < cnt; ++j) {
                        ptr[j] = alloc_in_slab(powl(2, i), NULL);
                        mu_check(ptr[j] != NULL);
                        *(int *)ptr[j] = j;
                }

                for (j = 0; j < cnt; j += 1) {
                        mu_check(*(int *)ptr[j] == j);
                        free_in_slab(ptr[j]);
                }

                free(ptr);
        }
}

MU_TEST_SUITE(test_suite)
{
        MU_RUN_TEST(test_buddy);
        MU_RUN_TEST(test_slab);
}

int main(int argc, char *argv[])
{
        MU_RUN_SUITE(test_suite);
        MU_REPORT();
        return minunit_status;
}
