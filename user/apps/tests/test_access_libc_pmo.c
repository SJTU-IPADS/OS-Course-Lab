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

/* This program is to be read / load by other test apps */
#include <chcore/syscall.h>
#include <string.h>
#include <stdio.h>
#include <chcore/defs.h>
#include <sys/auxv.h>
#include <bits/errno.h>
#include "test_tools.h"

int main(int argc, char *argv[])
{
        unsigned char *v0[8192];
        int ret = 0;
        int pmo_cap;
        unsigned long vaddr;

        pmo_cap = getauxval(AT_CHCORE_LIBC_PMO_CAP);
        /* test whether apps can write mapped libc pmo or not */
        ret = usys_write_pmo(pmo_cap, 0, v0, 7902);
        chcore_assert(ret == -ECAPBILITY, "PMO rights check doesn't work!");
        printf("No permission to write this pmo!\n");
        /* test if apps can map libc pmo VM_WRITE perm */
        vaddr = chcore_alloc_vaddr(PAGE_SIZE);
        ret = usys_map_pmo(SELF_CAP, pmo_cap, vaddr, VM_WRITE);
        chcore_assert(ret == -ECAPBILITY, "PMO rights check doesn't work!");
        printf("PMO rights don't contain this VM perssion!\n");
        pmo_cap = usys_create_pmo_with_rights(PAGE_SIZE, PMO_DATA, PMO_WRITE);
        chcore_assert(pmo_cap >= 0, "usys_create_pmo_with_rights ret %d", pmo_cap);
        /* test whether apps can read a PMO_WRITE only pmo or not */
        ret = usys_read_pmo(pmo_cap, 0, v0, PAGE_SIZE);
        chcore_assert(ret == -ECAPBILITY, "PMO rights check doesn't work!");
        printf("No permission to read this pmo!\n");
        /* test if apps can map libc pmo VM_READ | VM_WRITE perm to a PMO_WRITE pmo */
        ret = usys_map_pmo(SELF_CAP, pmo_cap, vaddr, VM_READ | VM_WRITE);
        chcore_assert(ret == -ECAPBILITY, "PMO rights check doesn't work!");
        printf("PMO rights don't contain these VM perssion!\n");
        return 0;
}
