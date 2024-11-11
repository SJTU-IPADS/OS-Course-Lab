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

#include "test_tools.h"

void delay(void)
{
        u64 begin, end;

        pmu_clear_cnt();
        begin = pmu_read_real_cycle();
        do {
                end = pmu_read_real_cycle();
        } while (end - begin < 500000000);
}

int write_log(char *filepath, const char *content)
{
        FILE *file;
        file = fopen(filepath, "a");
        chcore_assert(file, "open log file failed\n");
        fwrite(content, strlen(content) + 1, 1, file);
        fclose(file);
        return 0;
}

int expect_log(char *filepath, const char *content)
{
        FILE *file;
        char str[20];
        file = fopen(filepath, "a+");
        chcore_assert(file, "open log file failed\n");
        while (1) {
                rewind(file);
                fread(str, strlen(content) + 1, 1, file);
                if (strcmp(str, content) == 0)
                        break;
        }
        fclose(file);
        unlink(filepath);
        return 0;
}

#if !__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4
volatile int atomic_lock;
int ___atomic_fetch_add(volatile int *addr, int inc)
{
        int old_atomic_lock, old;
        do {
                __asm__ __volatile__("ldstub %1, %0"
                                     : "=r"(old_atomic_lock), "=m"(atomic_lock)
                                     : "m"(atomic_lock)
                                     : "memory");
        } while (old_atomic_lock);

        old = *addr;
        *addr = old + inc;

        atomic_lock = 0;
        __asm__ __volatile__("" ::: "memory");
        return old;
}
#endif
