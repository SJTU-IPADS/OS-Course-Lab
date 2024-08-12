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
#include <arch/machine/pmu.h>

#define LOCK_TEST_NUM 5000000

#include "tests.h"

volatile int rwlock_start_flag = 0;
volatile int rwlock_finish_flag = 0;

/* Reader Writer test count */
unsigned long rw_tst_cnt_0 = 0;
unsigned long rw_tst_cnt_1 = 0;

struct rwlock global_rwlock;

void tst_rwlock(void)
{
        u64 start = 0, end = 0;

        /* Init */
        if (smp_get_cpu_id() == 0) {
                rwlock_init(&global_rwlock);
                start = pmu_read_real_cycle();
        }

        /* ============ Start Barrier ============ */
        lock(&big_kernel_lock);
        rwlock_start_flag++;
        unlock(&big_kernel_lock);
        while (rwlock_start_flag != PLAT_CPU_NUM)
                ;
        /* ============ Start Barrier ============ */

        /* Reader Writer Lock */
        for (int i = 0; i < LOCK_TEST_NUM; i++) {
                if (i % 3) {
                        if (i == 1)
                                read_lock(&global_rwlock);
                        else
                                while (read_try_lock(&global_rwlock) != 0)
                                        ;
                        BUG_ON(rw_tst_cnt_0 != rw_tst_cnt_1);
                        read_unlock(&global_rwlock);
                } else {
                        if (rw_tst_cnt_0 % 2)
                                write_lock(&global_rwlock);
                        else
                                while (write_try_lock(&global_rwlock) != 0)
                                        ;
                        rw_tst_cnt_0++;
                        rw_tst_cnt_1++;
                        write_unlock(&global_rwlock);
                }
        }

        /* ============ Finish Barrier ============ */
        lock(&big_kernel_lock);
        rwlock_finish_flag++;
        unlock(&big_kernel_lock);
        while (rwlock_finish_flag != PLAT_CPU_NUM)
                ;
        /* ============ Finish Barrier ============ */

        /* Check */
        BUG_ON(rw_tst_cnt_0 != rw_tst_cnt_1);

        if (smp_get_cpu_id() == 0) {
                end = pmu_read_real_cycle();
                kinfo("[TEST] RWLOCK Performance %ld\n",
                      (end - start) / (LOCK_TEST_NUM * PLAT_CPU_NUM));
        }
}
