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
#include <math.h>
#include <assert.h>

#include <chcore/syscall.h>
#include <chcore/memory.h>

int main()
{
        unsigned long a1 = chcore_alloc_vaddr(1024);
        unsigned long a2 = chcore_alloc_vaddr(1024);
        assert(fabs(a2 - a1) >= 1024);
        chcore_free_vaddr(a1, 1024);
        chcore_free_vaddr(a2, 1024);

        int pmo1 = usys_create_pmo(4096, PMO_ANONYM);
        unsigned long *ptr1 =
                (unsigned long *)chcore_auto_map_pmo(pmo1, 4096, VM_READ | VM_WRITE);
        assert(ptr1 != NULL);
        *ptr1 = 1234;
        *(ptr1 + 123) = 5678;
        *(ptr1 + 4096 / sizeof(unsigned long) - 1) = 42;
        assert(*ptr1 == 1234);
        assert(*(ptr1 + 123) == 5678);
        assert(*(ptr1 + 4096 / sizeof(unsigned long) - 1) == 42);
        chcore_auto_unmap_pmo(pmo1, (unsigned long)ptr1, 4096);

        return 0;
}
