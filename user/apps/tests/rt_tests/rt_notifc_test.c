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

#include <inttypes.h>
#include "common.h"

#define SEND 0
#define RECV 1

struct test_options {
        u64 interval;
        u64 distance;
        u32 thread_num;
        u32 cpu_num;
        u32 priority;
        bool smp;
};

struct thread_stat {
        u64 cycles;
        u64 min;
        u64 max;
        u64 act;
        double avg;
        struct timespec before;
        struct timespec after;
};

struct thread_param {
        u64 interval;
        u32 affinity[2];
        u32 priority[2];
        u32 test_notifc;
        u32 sync_notifc;
        struct thread_stat *stat;
};

static void print_usage(void)
{
        printf("Usage:\n"
               "rt_notifc_test.bin <options>\n\n"
               "-t NUM    number of sender/receiver pairs, default = 1\n"
               "-c NUM    number of CPUs, default = 1\n"
               "-p PRIO   highest scheduling priority of thread, default = 1\n"
               "-i INTV   base interval of thread in us, default = 1000\n"
               "-d DIST   distance of thread intervals in us, default = 500\n"
               "-s        bind sender and receiver to different CPUs\n");
}

static void get_test_options(int argc, char *argv[],
                             struct test_options *options)
{
        int c;

        /* Default test options */
        options->thread_num = 1;
        options->cpu_num = 1;
        options->priority = 1;
        options->interval = 1000;
        options->distance = 500;
        options->smp = false;

        for (;;) {
                c = getopt(argc, argv, "t:c:p:i:d:sh");
                if (c == -1) {
                        break;
                }

                if (c == 's') {
                        options->smp = true;
                        continue;
                } else if (c == 'h') {
                        print_usage();
                        exit(0);
                } else if (optarg == NULL) {
                        continue;
                }

                switch (c) {
                case 't':
                        options->thread_num = atoi(optarg);
                        break;
                case 'c':
                        options->cpu_num = atoi(optarg);
                        break;
                case 'p':
                        options->priority = atoi(optarg);
                        break;
                case 'i':
                        options->interval = atoi(optarg);
                        break;
                case 'd':
                        options->distance = atoi(optarg);
                        break;
                default:
                        BUG_ON(1);
                }
        }
}

static void init_thread(u32 index, struct thread_param *param,
                        struct thread_stat *stat, struct test_options *options)
{
        param->priority[SEND] = options->priority >= 2 * index + 2 ?
                                        options->priority - 2 * index - 1 :
                                        1;
        param->priority[RECV] = options->priority >= 2 * index + 1 ?
                                        options->priority - 2 * index :
                                        1;
        param->affinity[SEND] =
                (options->smp ? 2 * index : index) % options->cpu_num;
        param->affinity[RECV] =
                (options->smp ? 2 * index + 1 : index) % options->cpu_num;
        param->interval = options->interval + options->distance * index;
        param->test_notifc = create_notifc();
        param->sync_notifc = create_notifc();
        param->stat = stat;

        stat->cycles = 0;
        stat->min = UINT32_MAX;
        stat->max = 0;
        stat->avg = 0.0;
}

static void print_thread(u32 index, struct thread_param *param,
                         struct thread_stat *stat)
{
        if (stat->cycles == 0) {
                printf("\n");
                return;
        }

        printf("T%02u->T%02u SP: %2u SA: %2u RP: %2u RA: %2u I: %5" PRIu64 " "
               "C: %7" PRIu64 " Min: %5" PRIu64 " Act: %5" PRIu64
               " Avg: %5" PRIu64 " Max: %5" PRIu64 "\n",
               2 * index,
               2 * index + 1,
               param->priority[SEND],
               param->affinity[SEND],
               param->priority[RECV],
               param->affinity[RECV],
               param->interval,
               stat->cycles,
               stat->min,
               stat->act,
               (u64)stat->avg,
               stat->max);
}

static void spin(u64 interval)
{
        u64 diff = 0;
        struct timespec before, after;

        clock_gettime(CLOCK_REALTIME, &before);
        while (diff < interval) {
                clock_gettime(CLOCK_REALTIME, &after);
                diff = timespec_diff(&after, &before);
        }
}

static void *send(void *arg)
{
        struct thread_param *param = arg;
        struct thread_stat *stat = param->stat;

        set_affinity(param->affinity[SEND]);

        for (;;) {
                usys_wait(param->sync_notifc, true, NULL);

                clock_gettime(CLOCK_REALTIME, &stat->before);
                usys_notify(param->test_notifc);

                spin(10); // Test whether the receiver preempts the sender
        }

        return NULL;
}

static void *recv(void *arg)
{
        u64 latency;
        double sum;
        struct timespec interval;
        struct thread_param *param = arg;
        struct thread_stat *stat = param->stat;

        set_affinity(param->affinity[RECV]);

        timespec_from_usec(&interval, param->interval);

        for (;;) {
                usys_notify(param->sync_notifc);
                usys_wait(param->test_notifc, true, NULL);
                clock_gettime(CLOCK_REALTIME, &stat->after);
                latency = timespec_diff(&stat->after, &stat->before);

                stat->act = latency;
                stat->min = MIN(stat->min, latency);
                stat->max = MAX(stat->max, latency);
                sum = stat->avg * stat->cycles + latency;
                stat->cycles++;
                stat->avg = sum / stat->cycles;

                clock_nanosleep(CLOCK_REALTIME, 0, &interval, NULL);
        }

        return NULL;
}

int main(int argc, char *argv[])
{
        u32 i;
        struct test_options options;
        struct thread_param *params;
        struct thread_stat *stats;

        get_test_options(argc, argv, &options);

        params = calloc(options.thread_num, sizeof(*params));
        stats = calloc(options.thread_num, sizeof(*stats));

        for (i = 0; i < options.thread_num; i++) {
                init_thread(i, &params[i], &stats[i], &options);
                create_thread(send, &params[i], params[i].priority[SEND]);
                create_thread(recv, &params[i], params[i].priority[RECV]);
        }

        for (;;) {
                for (i = 0; i < options.thread_num; i++) {
                        print_thread(i, &params[i], &stats[i]);
                }

                usleep(10000);

                printf("\033[%dA", options.thread_num);
        }

        return 0;
}
