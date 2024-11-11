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

#include <assert.h>
#include <chcore/memory.h>
#include <chcore/syscall.h>
#include <stdio.h>

int main()
{
        int pmo = usys_create_pmo(4096, PMO_ANONYM);
        unsigned long *ptr =
                (unsigned long *)chcore_auto_map_pmo(pmo, 4096, VM_READ | VM_WRITE);
        assert(ptr != NULL);
        printf("chcore_auto_map_pmo success.\n");
        chcore_auto_unmap_pmo(pmo, (unsigned long)ptr, 4096);
        printf("chcore_auto_unmap_pmo success.\n");
        printf("Test finish!\n");
        return 0;
}
