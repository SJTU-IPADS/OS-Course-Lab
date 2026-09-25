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
#include "minunit.h"

#undef PAGE_SHIFT
#undef PAGE_SIZE

#include <arch/mmu.h>

#undef phys_to_virt
#undef virt_to_phys
#define phys_to_virt(x) ((u64)x)
#define virt_to_phys(x) ((u64)x)

u8 pcid_disabled;

void *get_pages(int order)
{
        void *ptr;
        int err = posix_memalign(&ptr, 0x1000, 0x1000);
        if (err)
                return NULL;
        return ptr;
}

void free_page(void *page)
{
        mu_assert(page != NULL, "Freeing nullptr!");
        free(page);
}

void kfree(void *ptr)
{
        free(ptr);
}

#include "../../../../arch/x86_64/mm/page_table.c"

void set_ttbr0_el1(paddr_t p)
{
}
void flush_tlb(void)
{
}
void flush_tlb_all(void)
{
}

void printk(const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
}

#define RND_MAPPING_PAGES (10000)
#define RND_VA_MAX        (0x10000000)
#define RND_PA_MAX        (0x10000000)
#define RND_SEED          (1024)
#define DEFAULT_FLAGS     (3)
#define ALTER_FLAGS       (4)

static inline u64 rand_addr(u64 max)
{
        if (max == 0)
                return 0;
        return (rand() % 0x10000) * (rand() % 0x10000) % max;
}
static inline u64 rand_page_addr(u64 max)
{
        u64 ret;

        do {
                ret = rand_addr(max) & (~PAGE_MASK);
        } while (ret == 0);
        return ret;
}

void test_page_mappings(void *pgtbl, paddr_t *pas, vaddr_t *vas, int nr_pages)
{
        paddr_t pa;
        vaddr_t va;
        vmr_prop_t flags;
        int err;
        int i;
        int j;
        pte_t *entry;
        /* Query all pages  */
        for (i = 0; i < nr_pages; i++) {
                if (!vas[i])
                        continue;
                err = query_in_pgtbl(pgtbl, vas[i], &pa, &entry);
                if (err != 0) {
                        printf("vas[i]=%llx\n", (u64)vas[i]);
                        exit(-1);
                }
                mu_assert_int_eq(0, err);
                if (pa != pas[i]) {
                        printf("pa=0x%llx pas[i]=0x%llx\n", pa, pas[i]);
                }
                mu_check(pa == pas[i]);
        }

        /* Generate some VAs and query them */
        for (i = 0; i < nr_pages; i++) {
                va = rand_page_addr(RND_VA_MAX);
                err = query_in_pgtbl(pgtbl, va, &pa, &entry);
                for (j = 0; j < RND_MAPPING_PAGES; j++) {
                        if (vas[j] != va)
                                continue;
                        mu_assert_int_eq(0, err);
                        mu_check(pa == pas[j]);
                        break;
                }
                if (j == RND_MAPPING_PAGES)
                        mu_assert_int_eq(-ENOMAPPING, err);
        }
}

MU_TEST(test_map_unmap_page)
{
        /* Test both map_apge and unmap_page. */
        int err;
        paddr_t pa;
        vaddr_t va;
        vmr_prop_t flags;
        void *pgtbl;
        pte_t *entry;

        paddr_t *pas;
        vaddr_t *vas;
        int i;
        int j;
        long rss;

        pgtbl = get_pages(0);
        BUG_ON(pgtbl == NULL);

        va = 0x100000;
        err = query_in_pgtbl(pgtbl, va, &pa, &entry);
        mu_assert_int_eq(-ENOMAPPING, err);

        rss = 0;
        err = map_range_in_pgtbl(pgtbl, va, 0x100000, PAGE_SIZE, DEFAULT_FLAGS, &rss);
        mu_assert_int_eq(0, err);
        mu_assert_int_eq(PAGE_SIZE * (L3 + 1), rss);

        err = query_in_pgtbl(pgtbl, va, &pa, &entry);
        mu_assert_int_eq(0, err);
        mu_check(pa == 0x100000);
        // mu_check(flags == DEFAULT_FLAGS);

        rss = 0;
        err = unmap_range_in_pgtbl(pgtbl, va, PAGE_SIZE, &rss);
        mu_assert_int_eq(0, err);
        mu_assert_int_eq(-PAGE_SIZE * (L3 + 1), rss);

        err = query_in_pgtbl(pgtbl, va, &pa, &entry);
        mu_assert_int_eq(-ENOMAPPING, err);

        srand(RND_SEED);
        vas = malloc(sizeof(*vas) * RND_MAPPING_PAGES);
        pas = malloc(sizeof(*pas) * RND_MAPPING_PAGES);
        /* Generate and map all pages */
        for (i = 0; i < RND_MAPPING_PAGES; i++) {
        rerand:
                vas[i] = rand_page_addr(RND_VA_MAX);
                pas[i] = rand_page_addr(RND_PA_MAX);
                for (j = 0; j < i; j++) {
                        if (vas[i] == vas[j])
                                goto rerand;
                }
                printf("map: 0x%llx -> 0x%llx\n", vas[i], pas[i]);
                rss = 0;
                err = map_range_in_pgtbl(
                        pgtbl, vas[i], pas[i], PAGE_SIZE, DEFAULT_FLAGS, &rss);
                mu_assert_int_eq(0, err);
                mu_assert(rss >= PAGE_SIZE && rss <= PAGE_SIZE * (L3 + 1),
                          "unexpected rss");
        }

        test_page_mappings(pgtbl, pas, vas, RND_MAPPING_PAGES);

        /* Unmap some pages */
        for (i = 0; i < RND_MAPPING_PAGES; i++) {
                if (rand() & 1)
                        continue;
                printf("unmap: 0x%llx -> 0x%llx\n", vas[i], pas[i]);
                rss = 0;
                err = unmap_range_in_pgtbl(pgtbl, vas[i], PAGE_SIZE, &rss);
                mu_assert_int_eq(0, err);
                /* currently page table pages are not reclaimed */
                mu_assert(rss >= -PAGE_SIZE * (L3 + 1) && rss <= -PAGE_SIZE,
                          "unexpected rss");
                vas[i] = 0;
                pas[i] = 0;
        }

        test_page_mappings(pgtbl, pas, vas, RND_MAPPING_PAGES);

        /* Unmap remaining pages */
        for (i = 0; i < RND_MAPPING_PAGES; i++) {
                if (!vas[i])
                        continue;
                printf("unmap: 0x%llx -> 0x%llx\n", vas[i], pas[i]);
                rss = 0;
                err = unmap_range_in_pgtbl(pgtbl, vas[i], PAGE_SIZE, &rss);
                mu_assert_int_eq(0, err);
                mu_assert(rss >= -PAGE_SIZE * (L3 + 1) && rss <= -PAGE_SIZE,
                          "unexpected rss");
                vas[i] = 0;
                pas[i] = 0;
        }
        test_page_mappings(pgtbl, pas, vas, RND_MAPPING_PAGES);

        /* destroy pgtbl */
        free(pgtbl);
}

MU_TEST_SUITE(test_suite)
{
        MU_RUN_TEST(test_map_unmap_page);
}

int main(int argc, char *argv[])
{
        MU_RUN_SUITE(test_suite);
        MU_REPORT();
        return minunit_status;
}
