#include <arch/mmu.h>
#include <arch/mm/page_table.h>
#include <common/macro.h>
#include <common/types.h>
#include <common/util.h>
#include <lib/printk.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>

#include "tests.h"

TEST_SUITE(lab2, test_buddy)
{
        struct phys_mem_pool *pool = &global_mem[0];
        TEST("Init buddy")
        {
                u64 free_mem_size = get_free_mem_size_from_buddy(pool);
                ASSERT(pool->pool_phys_page_num == free_mem_size / PAGE_SIZE);
        }
        TEST("Allocate & free order 0")
        {
                u64 expect_free_mem = pool->pool_phys_page_num * PAGE_SIZE;
                struct page *page = buddy_get_pages(pool, 0);
                BUG_ON(page == NULL);
                ASSERT(page->order == 0 && page->allocated);
                expect_free_mem -= PAGE_SIZE;
                ASSERT(get_free_mem_size_from_buddy(pool) == expect_free_mem);
                buddy_free_pages(pool, page);
                expect_free_mem += PAGE_SIZE;
                ASSERT(get_free_mem_size_from_buddy(pool) == expect_free_mem);
        }
        TEST("Allocate & free each order")
        {
                u64 expect_free_mem = pool->pool_phys_page_num * PAGE_SIZE;
                struct page *page;
                for (int i = 0; i < BUDDY_MAX_ORDER; i++) {
                        page = buddy_get_pages(pool, i);
                        BUG_ON(page == NULL);
                        ASSERT(page->order == i && page->allocated);
                        expect_free_mem -= (1 << i) * PAGE_SIZE;
                        ASSERT(get_free_mem_size_from_buddy(pool)
                               == expect_free_mem);
                        buddy_free_pages(pool, page);
                        expect_free_mem += (1 << i) * PAGE_SIZE;
                        ASSERT(get_free_mem_size_from_buddy(pool)
                               == expect_free_mem);
                }
                for (int i = BUDDY_MAX_ORDER - 1; i >= 0; i--) {
                        page = buddy_get_pages(pool, i);
                        BUG_ON(page == NULL);
                        ASSERT(page->order == i && page->allocated);
                        expect_free_mem -= (1 << i) * PAGE_SIZE;
                        ASSERT(get_free_mem_size_from_buddy(pool)
                               == expect_free_mem);
                        buddy_free_pages(pool, page);
                        expect_free_mem += (1 << i) * PAGE_SIZE;
                        ASSERT(get_free_mem_size_from_buddy(pool)
                               == expect_free_mem);
                }
        }
        TEST("Allocate & free all orders")
        {
                u64 expect_free_mem = pool->pool_phys_page_num * PAGE_SIZE;
                struct page *pages[BUDDY_MAX_ORDER];
                for (int i = 0; i < BUDDY_MAX_ORDER; i++) {
                        pages[i] = buddy_get_pages(pool, i);
                        BUG_ON(pages[i] == NULL);
                        ASSERT(pages[i]->order == i);
                        expect_free_mem -= (1 << i) * PAGE_SIZE;
                        ASSERT(get_free_mem_size_from_buddy(pool)
                               == expect_free_mem);
                }
                for (int i = 0; i < BUDDY_MAX_ORDER; i++) {
                        buddy_free_pages(pool, pages[i]);
                        expect_free_mem += (1 << i) * PAGE_SIZE;
                        ASSERT(get_free_mem_size_from_buddy(pool)
                               == expect_free_mem);
                }
        }
        TEST("Allocate & free all memory")
        {
                u64 expect_free_mem = pool->pool_phys_page_num * PAGE_SIZE;
                struct page *page;
                for (int i = 0; i < pool->pool_phys_page_num; i++) {
                        page = buddy_get_pages(pool, 0);
                        BUG_ON(page == NULL);
                        ASSERT(page->order == 0);
                        expect_free_mem -= PAGE_SIZE;
                        ASSERT(get_free_mem_size_from_buddy(pool)
                               == expect_free_mem);
                }
                ASSERT(get_free_mem_size_from_buddy(pool) == 0);
                ASSERT(buddy_get_pages(pool, 0) == NULL);
                for (int i = 0; i < pool->pool_phys_page_num; i++) {
                        page = pool->page_metadata + i;
                        ASSERT(page->allocated);
                        buddy_free_pages(pool, page);
                        expect_free_mem += PAGE_SIZE;
                        ASSERT(get_free_mem_size_from_buddy(pool)
                               == expect_free_mem);
                }
                ASSERT(pool->pool_phys_page_num * PAGE_SIZE == expect_free_mem);
        }
        printk("[TEST] Buddy tests finished\n");
}

TEST_SUITE(lab2, test_kmalloc)
{
        TEST("kmalloc")
        {
                {
                        void *p = kmalloc(10);
                        ASSERT(p != NULL);
                        kfree(p);
                }
                {
                        int *p = (int *)kmalloc(100 * sizeof(int));
                        ASSERT(p != NULL);
                        for (int i = 0; i < 100; i++) {
                                p[i] = i;
                        }
                        for (int i = 0; i < 100; i++) {
                                ASSERT(p[i] == i);
                        }
                        kfree(p);
                }
                {
                        u8 *p = (u8 *)kzalloc(0x2000);
                        ASSERT(p != NULL);
                        for (int i = 0; i < 0x2000; i++) {
                                ASSERT(p[i] == 0);
                        }
                        kfree(p);
                }
        }
}

TEST_SUITE(lab2, test_page_table)
{
        vmr_prop_t flags = VMR_READ | VMR_WRITE;
        TEST("Map & unmap one page")
        {
                void *pgtbl = get_pages(0);
                paddr_t pa;
                pte_t *pte;
                int ret;

                memset(pgtbl, 0, PAGE_SIZE);
                ret = map_range_in_pgtbl(
                        pgtbl, 0x1001000, 0x1000, PAGE_SIZE, flags, NULL);
                ASSERT(ret == 0);

                ret = query_in_pgtbl(pgtbl, 0x1001000, &pa, &pte);
                ASSERT(ret == 0 && pa == 0x1000);
                ASSERT(pte && pte->l3_page.is_valid && pte->l3_page.is_page
                       && pte->l3_page.SH == INNER_SHAREABLE);
                ret = query_in_pgtbl(pgtbl, 0x1001050, &pa, &pte);
                ASSERT(ret == 0 && pa == 0x1050);

                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000, PAGE_SIZE, NULL);
                ASSERT(ret == 0);
                ret = query_in_pgtbl(pgtbl, 0x1001000, &pa, &pte);
                ASSERT(ret == -ENOMAPPING);

                free_page_table(pgtbl);
        }
        TEST("Map & unmap multiple pages")
        {
                void *pgtbl = get_pages(0);
                paddr_t pa;
                pte_t *pte;
                int ret;
                size_t nr_pages = 10;
                size_t len = PAGE_SIZE * nr_pages;

                memset(pgtbl, 0, PAGE_SIZE);
                ret = map_range_in_pgtbl(
                        pgtbl, 0x1001000, 0x1000, len, flags, NULL);
                ASSERT(ret == 0);
                ret = map_range_in_pgtbl(
                        pgtbl, 0x1001000 + len, 0x1000 + len, len, flags, NULL);
                ASSERT(ret == 0);

                for (int i = 0; i < nr_pages * 2; i++) {
                        ret = query_in_pgtbl(
                                pgtbl, 0x1001050 + i * PAGE_SIZE, &pa, &pte);
                        ASSERT(ret == 0 && pa == 0x1050 + i * PAGE_SIZE);
                        ASSERT(pte && pte->l3_page.is_valid
                               && pte->l3_page.is_page);
                }

                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000, len, NULL);
                ASSERT(ret == 0);
                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000 + len, len, NULL);
                ASSERT(ret == 0);

                for (int i = 0; i < nr_pages * 2; i++) {
                        ret = query_in_pgtbl(
                                pgtbl, 0x1001050 + i * PAGE_SIZE, &pa, &pte);
                        ASSERT(ret == -ENOMAPPING);
                }

                free_page_table(pgtbl);
        }
        TEST("Map & unmap huge pages")
        {
                void *pgtbl = get_pages(0);
                paddr_t pa;
                pte_t *pte;
                int ret;
                /* 1GB + 4MB + 40KB */
                size_t len = (1 << 30) + (4 << 20) + 10 * PAGE_SIZE;

                memset(pgtbl, 0, PAGE_SIZE);
                ret = map_range_in_pgtbl(
                        pgtbl, 0x100000000, 0x100000000, len, flags, NULL);
                ASSERT(ret == 0);
                ret = map_range_in_pgtbl(pgtbl,
                                         0x100000000 + len,
                                         0x100000000 + len,
                                         len,
                                         flags,
                                         NULL);
                ASSERT(ret == 0);

                for (vaddr_t va = 0x100000000; va < 0x100000000 + len * 2;
                     va += 5 * PAGE_SIZE + 0x100) {
                        ret = query_in_pgtbl(pgtbl, va, &pa, &pte);
                        ASSERT(ret == 0 && pa == va);
                }

                ret = unmap_range_in_pgtbl(pgtbl, 0x100000000, len, NULL);
                ASSERT(ret == 0);
                ret = unmap_range_in_pgtbl(pgtbl, 0x100000000 + len, len, NULL);
                ASSERT(ret == 0);

                for (vaddr_t va = 0x100000000; va < 0x100000000 + len;
                     va += 5 * PAGE_SIZE + 0x100) {
                        ret = query_in_pgtbl(pgtbl, va, &pa, &pte);
                        ASSERT(ret == -ENOMAPPING);
                }

                free_page_table(pgtbl);
        }
        printk("[TEST] Page table tests finished\n");
}

TEST_SUITE(lab2, test_pm_usage)
{
        vmr_prop_t flags = VMR_READ | VMR_WRITE;
        TEST("Compute physical memory-1")
        {
                void *pgtbl = get_pages(0);
                paddr_t pa;
                pte_t *pte;
                int ret;
                long rss = 0;

                memset(pgtbl, 0, PAGE_SIZE);
                ret = map_range_in_pgtbl(
                        pgtbl, 0x1001000, 0x1000, PAGE_SIZE, flags, &rss);
#ifdef CHCORE_KERNEL_PM_USAGE_TEST
                ASSERT(rss == PAGE_SIZE * (3 + 1));
#endif

                ASSERT(ret == 0);
                ret = query_in_pgtbl(pgtbl, 0x1001000, &pa, &pte);
                ASSERT(ret == 0 && pa == 0x1000);
                ASSERT(pte && pte->l3_page.is_valid && pte->l3_page.is_page
                       && pte->l3_page.SH == INNER_SHAREABLE);
                ret = query_in_pgtbl(pgtbl, 0x1001050, &pa, &pte);
                ASSERT(ret == 0 && pa == 0x1050);
                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000, PAGE_SIZE, &rss);
#ifdef CHCORE_KERNEL_PM_USAGE_TEST
                ASSERT(rss == 0);
#endif
                ASSERT(ret == 0);
                ret = query_in_pgtbl(pgtbl, 0x1001000, &pa, &pte);
                ASSERT(ret == -ENOMAPPING);

                free_page_table(pgtbl);
        }
        TEST("Compute physical memory-2")
        {
                void *pgtbl = get_pages(0);
                paddr_t pa;
                pte_t *pte;
                int ret;
                size_t nr_pages = 10;
                size_t len = PAGE_SIZE * nr_pages;
                long rss_1 = 0, rss_2 = 0;

                memset(pgtbl, 0, PAGE_SIZE);
                ret = map_range_in_pgtbl(
                        pgtbl, 0x1001000, 0x1000, len, flags, &rss_1);
                ASSERT(ret == 0);
                ret = map_range_in_pgtbl(pgtbl,
                                         0x1001000 + len,
                                         0x1000 + len,
                                         len,
                                         flags,
                                         &rss_2);
                ASSERT(ret == 0);
#ifdef CHCORE_KERNEL_PM_USAGE_TEST
                ASSERT(rss_1 == PAGE_SIZE * (3 + nr_pages));
                ASSERT(rss_2 == PAGE_SIZE * (nr_pages));
#endif

                for (int i = 0; i < nr_pages * 2; i++) {
                        ret = query_in_pgtbl(
                                pgtbl, 0x1001050 + i * PAGE_SIZE, &pa, &pte);
                        ASSERT(ret == 0 && pa == 0x1050 + i * PAGE_SIZE);
                        ASSERT(pte && pte->l3_page.is_valid
                               && pte->l3_page.is_page);
                }

                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000, len, &rss_1);
                ASSERT(ret == 0);
                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000 + len, len, &rss_2);
                ASSERT(ret == 0);
#if defined(CHCORE_KERNEL_PM_USAGE_TEST)
                ASSERT(0 == (rss_1 + rss_2));
#endif
                for (int i = 0; i < nr_pages * 2; i++) {
                        ret = query_in_pgtbl(
                                pgtbl, 0x1001050 + i * PAGE_SIZE, &pa, &pte);
                        ASSERT(ret == -ENOMAPPING);
                }

                free_page_table(pgtbl);
        }
        TEST("Compute physical memory-3")
        {
                void *pgtbl = get_pages(0);
                paddr_t pa;
                pte_t *pte;
                int ret;
                /* 1GB + 4MB + 40KB */
                size_t len = (1 << 30) + (4 << 20) + 10 * PAGE_SIZE;
                long rss_1 = 0, rss_2 = 0;

                memset(pgtbl, 0, PAGE_SIZE);
                ret = map_range_in_pgtbl(
                        pgtbl, 0x100000000, 0x100000000, len, flags, &rss_1);
                ASSERT(ret == 0);
                ret = map_range_in_pgtbl(pgtbl,
                                         0x100000000 + len,
                                         0x100000000 + len,
                                         len,
                                         flags,
                                         &rss_2);
                ASSERT(ret == 0);

#ifdef CHCORE_KERNEL_PM_USAGE_TEST
                ASSERT(rss_1 == (PAGE_SIZE * (518) + len));
#endif
                for (vaddr_t va = 0x100000000; va < 0x100000000 + len * 2;
                     va += 5 * PAGE_SIZE + 0x100) {
                        ret = query_in_pgtbl(pgtbl, va, &pa, &pte);
                        ASSERT(ret == 0 && pa == va);
                }
                ret = unmap_range_in_pgtbl(pgtbl, 0x100000000, len, &rss_1);
                ASSERT(ret == 0);
                ret = unmap_range_in_pgtbl(
                        pgtbl, 0x100000000 + len, len, &rss_2);
                ASSERT(ret == 0);
#if CHCORE_KERNEL_PM_USAGE_TEST
                ASSERT(0 == (rss_1 + rss_2));
#endif
                for (vaddr_t va = 0x100000000; va < 0x100000000 + len;
                     va += 5 * PAGE_SIZE + 0x100) {
                        ret = query_in_pgtbl(pgtbl, va, &pa, &pte);
                        ASSERT(ret == -ENOMAPPING);
                }

                free_page_table(pgtbl);
        }
        printk("[TEST] physical memory calculate tests finished\n");
}
