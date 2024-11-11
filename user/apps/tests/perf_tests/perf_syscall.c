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
#include <stdlib.h>
#include <chcore/syscall.h>
#include <chcore/thread.h>
#include <sched.h>
#include <inttypes.h>

#include "perf_tools.h"

#define PERF_NUM PERF_NUM_LARGE

int main(int argc, char *argv[])
{
        if (bind_thread_to_core(2) < 0)
                error("sched_setaffinity failed!\n");

        perf_time_t *start_perf_time = malloc(sizeof(perf_time_t));
        perf_time_t *end_perf_time = malloc(sizeof(perf_time_t));

        int i = 0;

        info("Start measure syscall\n");

        record_time(start_perf_time);
        for (i = 0; i < PERF_NUM; i++)
                usys_empty_syscall();

        record_time(end_perf_time);
        printf("Perf Syscall Result:\n");
        print_perf_result(start_perf_time, end_perf_time, PERF_NUM);

        free(start_perf_time);
        free(end_perf_time);

        return 0;
}
