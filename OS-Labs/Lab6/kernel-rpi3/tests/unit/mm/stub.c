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

int physmem_map_num = 2;

void printk(const char *fmt, ...)
{
        va_list va;

        va_start(va, fmt);
        vprintf(fmt, va);
        va_end(va);
}

struct lock;

int lock_init(struct lock *lock)
{
        return 0;
}
int lock(struct lock *lock)
{
        return 0;
}
int unlock(struct lock *lock)
{
        return 0;
}
