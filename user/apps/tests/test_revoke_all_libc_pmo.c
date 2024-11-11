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
        int ret = 0;
        int pmo_cap;

        pmo_cap = getauxval(AT_CHCORE_LIBC_PMO_CAP);
        ret = usys_revoke_cap(pmo_cap, true);
        chcore_assert(ret = -ECAPBILITY, "libc pmo cannot be revoked_all ret %d\n", ret);

        return ret;
}
