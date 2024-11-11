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

#include <chcore/memory.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

int main()
{
        unsigned long a1 = chcore_alloc_vaddr(1024);
        unsigned long a2 = chcore_alloc_vaddr(1024);
        assert(fabs(a2 - a1) >= 1024);
        printf("chcore_alloc_vaddr success.\n");
        chcore_free_vaddr(a1, 2048);
        printf("chcore_free_vaddr success.\n");
        printf("Test finish!\n");
        return 0;
}
