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

#include "chcore/container/rbtree_plus.h"
#include "chcore/memory.h"
#include <minunit.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../../porting/overrides/src/chcore-port/rbtree.c"

#include "../../porting/overrides/src/chcore-port/rbtree_plus.c"

#include "../../porting/overrides/src/chcore-port/memory.c"

#define CHCORE_ASLR 0

int usys_map_pmo(cap_t cap_group_cap, cap_t pmo_cap, unsigned long addr, unsigned long perm)
{
    return 0;
}

int usys_unmap_pmo(cap_t cap_group_cap, cap_t pmo_cap, unsigned long addr)
{
    return 0;
}

int usys_map_pmo_with_length(cap_t pmo_cap, vaddr_t addr, unsigned long perm,
                             size_t length)
{
    return 0;
}

int usys_unmap_pmo_with_length(cap_t pmo_cap, unsigned long addr, size_t size)
{
    return 0;
}

cap_t usys_create_pmo(unsigned long size, unsigned long flags)
{
    return 0;
}

int usys_get_phys_addr(void* addr, unsigned long *paddr)
{
    return 0;
}

int usys_revoke_cap(cap_t cap, bool revoke_copy)
{
    return 0;
}

struct range {
        vaddr_t start;
        vaddr_t len;
};

struct range ranges[] = {
        {MEM_AUTO_ALLOC_BASE + 0x1000, 0x1000},
        {MEM_AUTO_ALLOC_BASE + 0x3000, MEM_AUTO_ALLOC_REGION_SIZE - 0x6000},
        {MEM_AUTO_ALLOC_BASE + MEM_AUTO_ALLOC_REGION_SIZE - 0x2000, 0x1000}};

MU_TEST(vaddr_alloc_basic)
{
        struct user_vmr* vmr;
        vaddr_t ret;

        ret = chcore_alloc_vaddr(0x1000);
        mu_check(ret == MEM_AUTO_ALLOC_BASE);

        chcore_free_vaddr(ret, 0x1000);

        for (int i = 0; i < sizeof(ranges) / sizeof(struct range); i++) {
                vmr = malloc(sizeof(*vmr));
                vmr->start = ranges[i].start;
                vmr->len = ranges[i].len;
                rbp_insert(&user_mm.user_vmr_tree, &vmr->node, less_vmr_node);
        }

        ret = chcore_alloc_vaddr(0x1000);
        mu_check(ret == MEM_AUTO_ALLOC_BASE);

        ret = chcore_alloc_vaddr(0x1000);
        mu_check(ret == MEM_AUTO_ALLOC_BASE + 0x2000);

        ret = chcore_alloc_vaddr(0x1000);
        mu_check(ret
                 == MEM_AUTO_ALLOC_BASE + MEM_AUTO_ALLOC_REGION_SIZE
                            - 0x3000);

        ret = chcore_alloc_vaddr(0x1000);
        mu_check(ret
                 == MEM_AUTO_ALLOC_BASE + MEM_AUTO_ALLOC_REGION_SIZE
                            - 0x1000);

        chcore_free_vaddr(MEM_AUTO_ALLOC_BASE, 0x1000);

        chcore_free_vaddr(MEM_AUTO_ALLOC_BASE + MEM_AUTO_ALLOC_REGION_SIZE
                                      - 0x1000,
                              0x1000);

        ret = chcore_alloc_vaddr(0x800);

        mu_check(ret
                 == MEM_AUTO_ALLOC_BASE + MEM_AUTO_ALLOC_REGION_SIZE
                            - 0x1000);

        ret = chcore_alloc_vaddr(0x1000);

        mu_check(ret == MEM_AUTO_ALLOC_BASE);

        ret = chcore_alloc_vaddr(0x1000);

        mu_check(ret == 0);
}

#define ALLOC_SIZE  (0x1000UL << (3 * sizeof(long)))
#define TEST_NUM    100
#define MAX_REGIONS (MEM_AUTO_ALLOC_REGION_SIZE / ALLOC_SIZE)

MU_TEST(vaddr_alloc_batch)
{
        vaddr_t ret;
        vaddr_t start = MEM_AUTO_ALLOC_BASE;
        vaddr_t end = MEM_AUTO_ALLOC_BASE + MEM_AUTO_ALLOC_REGION_SIZE;
        int allocated_cnt = 0;
        int remains;

        chcore_free_vaddr(start, end - start);
        mu_check(rbp_first(&user_mm.user_vmr_tree) == NULL);

        while (start < end) {
                ret = chcore_alloc_vaddr(ALLOC_SIZE);
                mu_assert_int_eq(ret, start);
                start += ALLOC_SIZE;
        }

        ret = chcore_alloc_vaddr(ALLOC_SIZE);
        mu_check(ret == 0);

        allocated_cnt = MAX_REGIONS;

        srand(time(NULL));
        for (int i = 0; i < TEST_NUM; i++) {
                start = MEM_AUTO_ALLOC_BASE;

                while (start < end) {
                        if (rand() & 1) {
                                chcore_free_vaddr(start, ALLOC_SIZE);
                                allocated_cnt--;
                        }
                        start += ALLOC_SIZE;
                }

                remains = MAX_REGIONS - allocated_cnt;

                for (int j = 0; j < remains; j++) {
                        ret = chcore_alloc_vaddr(ALLOC_SIZE);
                        mu_check(ret);
                        allocated_cnt++;
                }

                ret = chcore_alloc_vaddr(ALLOC_SIZE);
                mu_check(ret == 0);
        }
}

int main()
{
        MU_RUN_TEST(vaddr_alloc_basic);
        MU_RUN_TEST(vaddr_alloc_batch);
        MU_REPORT();
        return minunit_status;
}
