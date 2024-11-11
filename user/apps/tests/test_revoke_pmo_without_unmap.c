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

#define _GNU_SOURCE

#include <stdio.h>
#include <chcore/memory.h>
#include <chcore/syscall.h>
#include <assert.h>

#define PMO_SIZE (128 * 1024)

int main(int argc, char *argv[], char *envp[])
{
        cap_t pmo;
        unsigned long vaddr, vaddr1;
        int i;

        pmo = usys_create_pmo(PMO_SIZE, PMO_ANONYM);
        vaddr = chcore_alloc_vaddr(PMO_SIZE);
        usys_map_pmo(SELF_CAP, pmo, vaddr, VM_READ | VM_WRITE);

        vaddr1 = chcore_alloc_vaddr(PMO_SIZE);
        usys_map_pmo(SELF_CAP, pmo, vaddr1, VM_READ | VM_WRITE);

        printf("accessing...\n");
        // access
        for (i = 0; i < PMO_SIZE; i += (1 << 12)) {
                *(long *)((char *)vaddr + i) = i;
        }
        for (i = 0; i < PMO_SIZE; i += (1 << 12)) {
                assert(*(long *)((char *)vaddr1 + i) == i);
        }

        printf("usys_revoke_cap...\n");
        usys_revoke_cap(pmo, false);

        printf("accessing...(page fault but not BUG in kernel)\n");
        // access
        for (i = 0; i < PMO_SIZE; i += (1 << 12)) {
                ((char *)vaddr1)[i] = 0;
        }
        for (i = 0; i < PMO_SIZE; i += (1 << 12)) {
                ((char *)vaddr)[i] = 0;
        }
        
        return 0;
}
