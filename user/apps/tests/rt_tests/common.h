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

#pragma once

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <errno.h>
#include <chcore/type.h>
#include <chcore/bug.h>
#include <chcore/syscall.h>

#define USEC_PER_SEC  1000000
#define NSEC_PER_USEC 1000

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

static inline void timespec_from_usec(struct timespec *ts, u64 usec)
{
        ts->tv_sec = usec / USEC_PER_SEC;
        ts->tv_nsec = (usec % USEC_PER_SEC) * NSEC_PER_USEC;
}

static inline u64 timespec_diff(struct timespec *lhs, struct timespec *rhs)
{
        u64 diff;

        diff = USEC_PER_SEC * (lhs->tv_sec - rhs->tv_sec);
        diff += (lhs->tv_nsec - rhs->tv_nsec) / NSEC_PER_USEC;

        return diff;
}

static inline void fatal(char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        fprintf(stderr, "FATAL: ");
        vfprintf(stderr, fmt, ap);
        va_end(ap);

        exit(-1);
}

static inline u32 create_notifc(void)
{
        int notifc = usys_create_notifc();

        if (notifc < 0) {
                fatal("usys_create_notifc() failed, ret = %d\n", notifc);
        }

        return notifc;
}

static inline void create_thread(void *(*routine)(void *), void *arg,
                                 u32 priority)
{
        int ret;
        pthread_t pthread;
        pthread_attr_t pthread_attr;
        struct sched_param sched_param;

        sched_param.sched_priority = priority;
        pthread_attr_init(&pthread_attr);
        pthread_attr_setinheritsched(&pthread_attr, 1);
        pthread_attr_setschedparam(&pthread_attr, &sched_param);

        ret = pthread_create(&pthread, &pthread_attr, routine, arg);
        if (ret < 0) {
                fatal("pthread_create() failed, ret = %d\n", ret);
        }
}

static inline void set_affinity(u32 affinity)
{
        int ret;
        cpu_set_t cpu_set;

        CPU_ZERO(&cpu_set);
        CPU_SET(affinity, &cpu_set);

        ret = sched_setaffinity(0, sizeof(cpu_set), &cpu_set);
        if (ret) {
                fatal("sched_setaffinity() failed, ret = %d\n", ret);
        }

        sched_yield();
}
