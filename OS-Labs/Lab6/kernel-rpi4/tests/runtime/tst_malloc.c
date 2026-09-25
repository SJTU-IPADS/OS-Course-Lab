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

#include <common/lock.h>
#include <common/kprint.h>
#include <arch/machine/smp.h>
#include <common/macro.h>
#include <mm/kmalloc.h>

#include "tests.h"

#include <arch/machine/pmu.h>

#define MALLOC_TEST_NUM   4096
#define MALLOC_TEST_ROUND 10

volatile int malloc_start_flag = 0;
volatile int malloc_finish_flag = 0;

void tst_malloc(void)
{
        char **buf;
        unsigned long start, end;

        buf = (char **)kmalloc(sizeof(char *) * MALLOC_TEST_NUM);

        /* ============ Start Barrier ============ */
        lock(&big_kernel_lock);
        malloc_start_flag++;
        unlock(&big_kernel_lock);
        while (malloc_start_flag != PLAT_CPU_NUM)
                ;
        /* ============ Start Barrier ============ */

        start = pmu_read_real_cycle();

        for (int round = 0; round < MALLOC_TEST_ROUND; round++) {
                for (int i = 0; i < MALLOC_TEST_NUM; i++) {
                        int size = 0x1 + i;
                        buf[i] = kmalloc(size);
                        BUG_ON(!buf[i]);
                        for (int j = 0; j < size; j++) {
                                buf[i][j] = (char)(i + size);
                        }
                }

                for (int i = 0; i < MALLOC_TEST_NUM; i++) {
                        int size = 0x1 + i;
                        for (int j = 0; j < size; j++) {
                                BUG_ON(buf[i][j] != (char)(i + size));
                        }
                        kfree(buf[i]);
                }
        }

        end = pmu_read_real_cycle();

        /* ============ Finish Barrier ============ */
        lock(&big_kernel_lock);
        kinfo("CPU: %d, kmalloc test takes %ld cycles.\n",
              smp_get_cpu_id(),
              end - start);
        malloc_finish_flag++;
        unlock(&big_kernel_lock);
        while (malloc_finish_flag != PLAT_CPU_NUM)
                ;
        /* ============ Finish Barrier ============ */

        kfree(buf);

        if (smp_get_cpu_id() == 0) {
                kinfo("[TEST] malloc succ!\n");
        }
}
