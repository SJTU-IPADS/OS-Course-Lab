#include <arch/mmu.h>
#include <arch/mm/page_table.h>
#include <common/macro.h>
#include <common/types.h>
#include <common/util.h>
#include <lib/printk.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>

#include "tests.h"

void lab2_test_buddy(void)
{
        struct phys_mem_pool *pool = &global_mem[0];
        {
                u64 free_mem_size = get_free_mem_size_from_buddy(pool);
                lab_check(pool->pool_phys_page_num == free_mem_size / PAGE_SIZE,
                          "Init buddy");
        }
        {
                bool ok = true;
                u64 expect_free_mem = pool->pool_phys_page_num * PAGE_SIZE;
                struct page *page = buddy_get_pages(pool, 0);
                BUG_ON(page == NULL);
                lab_assert(page->order == 0 && page->allocated);
                expect_free_mem -= PAGE_SIZE;
                lab_assert(get_free_mem_size_from_buddy(pool)
                           == expect_free_mem);
                buddy_free_pages(pool, page);
                expect_free_mem += PAGE_SIZE;
                lab_assert(get_free_mem_size_from_buddy(pool)
                           == expect_free_mem);
                lab_check(ok, "Allocate & free order 0");
        }
        {
                bool ok = true;
                u64 expect_free_mem = pool->pool_phys_page_num * PAGE_SIZE;
                struct page *page;
                for (int i = 0; i < BUDDY_MAX_ORDER; i++) {
                        page = buddy_get_pages(pool, i);
                        BUG_ON(page == NULL);
                        lab_assert(page->order == i && page->allocated);
                        expect_free_mem -= (1 << i) * PAGE_SIZE;
                        lab_assert(get_free_mem_size_from_buddy(pool)
                                   == expect_free_mem);
                        buddy_free_pages(pool, page);
                        expect_free_mem += (1 << i) * PAGE_SIZE;
                        lab_assert(get_free_mem_size_from_buddy(pool)
                                   == expect_free_mem);
                }
                for (int i = BUDDY_MAX_ORDER - 1; i >= 0; i--) {
                        page = buddy_get_pages(pool, i);
                        BUG_ON(page == NULL);
                        lab_assert(page->order == i && page->allocated);
                        expect_free_mem -= (1 << i) * PAGE_SIZE;
                        lab_assert(get_free_mem_size_from_buddy(pool)
                                   == expect_free_mem);
                        buddy_free_pages(pool, page);
                        expect_free_mem += (1 << i) * PAGE_SIZE;
                        lab_assert(get_free_mem_size_from_buddy(pool)
                                   == expect_free_mem);
                }
                lab_check(ok, "Allocate & free each order");
        }
        {
                bool ok = true;
                u64 expect_free_mem = pool->pool_phys_page_num * PAGE_SIZE;
                struct page *pages[BUDDY_MAX_ORDER];
                for (int i = 0; i < BUDDY_MAX_ORDER; i++) {
                        pages[i] = buddy_get_pages(pool, i);
                        BUG_ON(pages[i] == NULL);
                        lab_assert(pages[i]->order == i);
                        expect_free_mem -= (1 << i) * PAGE_SIZE;
                        lab_assert(get_free_mem_size_from_buddy(pool)
                                   == expect_free_mem);
                }
                for (int i = 0; i < BUDDY_MAX_ORDER; i++) {
                        buddy_free_pages(pool, pages[i]);
                        expect_free_mem += (1 << i) * PAGE_SIZE;
                        lab_assert(get_free_mem_size_from_buddy(pool)
                                   == expect_free_mem);
                }
                lab_check(ok, "Allocate & free all orders");
        }
        {
                bool ok = true;
                u64 expect_free_mem = pool->pool_phys_page_num * PAGE_SIZE;
                struct page *page;
                for (int i = 0; i < pool->pool_phys_page_num; i++) {
                        page = buddy_get_pages(pool, 0);
                        BUG_ON(page == NULL);
                        lab_assert(page->order == 0);
                        expect_free_mem -= PAGE_SIZE;
                        lab_assert(get_free_mem_size_from_buddy(pool)
                                   == expect_free_mem);
                }
                lab_assert(get_free_mem_size_from_buddy(pool) == 0);
                lab_assert(buddy_get_pages(pool, 0) == NULL);
                for (int i = 0; i < pool->pool_phys_page_num; i++) {
                        page = pool->page_metadata + i;
                        lab_assert(page->allocated);
                        buddy_free_pages(pool, page);
                        expect_free_mem += PAGE_SIZE;
                        lab_assert(get_free_mem_size_from_buddy(pool)
                                   == expect_free_mem);
                }
                lab_assert(pool->pool_phys_page_num * PAGE_SIZE
                           == expect_free_mem);
                lab_check(ok, "Allocate & free all memory");
        }
        printk("[TEST] Buddy tests finished\n");
}

void lab2_test_kmalloc(void)
{
        /* This can test both boot page table and buddy system. */
        bool ok = true;
        {
                void *p = kmalloc(10);
                BUG_ON(p == NULL);
                kfree(p);
        }
        {
                int *p = (int *)kmalloc(100 * sizeof(int));
                BUG_ON(p == NULL);
                for (int i = 0; i < 100; i++) {
                        p[i] = i;
                }
                for (int i = 0; i < 100; i++) {
                        lab_assert(p[i] == i);
                }
                kfree(p);
        }
        {
                u8 *p = (u8 *)kzalloc(0x2000);
                BUG_ON(p == NULL);
                for (int i = 0; i < 0x2000; i++) {
                        lab_assert(p[i] == 0);
                }
                kfree(p);
        }
        lab_check(ok, "kmalloc");
}

void lab2_test_page_table(void)
{
        vmr_prop_t flags = VMR_READ | VMR_WRITE;
        {
                bool ok = true;
                void *pgtbl = get_pages(0);
                memset(pgtbl, 0, PAGE_SIZE);
                paddr_t pa;
                pte_t *pte;
                int ret;

                ret = map_range_in_pgtbl(
                        pgtbl, 0x1001000, 0x1000, PAGE_SIZE, flags, NULL);
                lab_assert(ret == 0);

                ret = query_in_pgtbl(pgtbl, 0x1001000, &pa, &pte);
                lab_assert(ret == 0 && pa == 0x1000);
                lab_assert(pte && pte->l3_page.is_valid && pte->l3_page.is_page
                           && pte->l3_page.SH == INNER_SHAREABLE);
                ret = query_in_pgtbl(pgtbl, 0x1001050, &pa, &pte);
                lab_assert(ret == 0 && pa == 0x1050);

                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000, PAGE_SIZE, NULL);
                lab_assert(ret == 0);
                ret = query_in_pgtbl(pgtbl, 0x1001000, &pa, &pte);
                lab_assert(ret == -ENOMAPPING);

                free_page_table(pgtbl);
                lab_check(ok, "Map & unmap one page");
        }
        {
                bool ok = true;
                void *pgtbl = get_pages(0);
                memset(pgtbl, 0, PAGE_SIZE);
                paddr_t pa;
                pte_t *pte;
                int ret;
                size_t nr_pages = 10;
                size_t len = PAGE_SIZE * nr_pages;

                ret = map_range_in_pgtbl(pgtbl, 0x1001000, 0x1000, len, flags, NULL);
                lab_assert(ret == 0);
                ret = map_range_in_pgtbl(
                        pgtbl, 0x1001000 + len, 0x1000 + len, len, flags, NULL);
                lab_assert(ret == 0);

                for (int i = 0; i < nr_pages * 2; i++) {
                        ret = query_in_pgtbl(
                                pgtbl, 0x1001050 + i * PAGE_SIZE, &pa, &pte);
                        lab_assert(ret == 0 && pa == 0x1050 + i * PAGE_SIZE);
                        lab_assert(pte && pte->l3_page.is_valid
                                   && pte->l3_page.is_page);
                }

                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000, len, NULL);
                lab_assert(ret == 0);
                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000 + len, len, NULL);
                lab_assert(ret == 0);

                for (int i = 0; i < nr_pages * 2; i++) {
                        ret = query_in_pgtbl(
                                pgtbl, 0x1001050 + i * PAGE_SIZE, &pa, &pte);
                        lab_assert(ret == -ENOMAPPING);
                }

                free_page_table(pgtbl);
                lab_check(ok, "Map & unmap multiple pages");
        }
        {
                bool ok = true;
                void *pgtbl = get_pages(0);
                memset(pgtbl, 0, PAGE_SIZE);
                paddr_t pa;
                pte_t *pte;
                int ret;
                /* 1GB + 4MB + 40KB */
                size_t len = (1 << 30) + (4 << 20) + 10 * PAGE_SIZE;

                ret = map_range_in_pgtbl(
                        pgtbl, 0x100000000, 0x100000000, len, flags, NULL);
                lab_assert(ret == 0);
                ret = map_range_in_pgtbl(pgtbl,
                                         0x100000000 + len,
                                         0x100000000 + len,
                                         len,
                                         flags, NULL);
                lab_assert(ret == 0);

                for (vaddr_t va = 0x100000000; va < 0x100000000 + len * 2;
                     va += 5 * PAGE_SIZE + 0x100) {
                        ret = query_in_pgtbl(pgtbl, va, &pa, &pte);
                        lab_assert(ret == 0 && pa == va);
                }

                ret = unmap_range_in_pgtbl(pgtbl, 0x100000000, len, NULL);
                lab_assert(ret == 0);
                ret = unmap_range_in_pgtbl(pgtbl, 0x100000000 + len, len, NULL);
                lab_assert(ret == 0);

                for (vaddr_t va = 0x100000000; va < 0x100000000 + len;
                     va += 5 * PAGE_SIZE + 0x100) {
                        ret = query_in_pgtbl(pgtbl, va, &pa, &pte);
                        lab_assert(ret == -ENOMAPPING);
                }

                free_page_table(pgtbl);
                lab_check(ok, "Map & unmap huge range");
        }
        printk("[TEST] Page table tests finished\n");
}