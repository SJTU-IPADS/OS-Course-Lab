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

struct test_options {
        u64 interval;
        u64 distance;
        u32 thread_num;
        u32 cpu_num;
        u32 priority;
};

struct thread_stat {
        u64 cycles;
        u64 min;
        u64 max;
        u64 act;
        double avg;
};

struct thread_param {
        u32 affinity;
        u32 priority;
        u64 interval;
        struct thread_stat *stat;
};

static void print_usage(void)
{
        printf("Usage:\n"
               "rt_sleep_test.bin <options>\n\n"
               "-t NUM    number of threads, default = 1\n"
               "-c NUM    number of CPUs, default = 1\n"
               "-p PRIO   highest scheduling priority of thread, default = 1\n"
               "-i INTV   base interval of thread in us, default = 1000\n"
               "-d DIST   distance of thread intervals in us, default = 500\n");
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

        /* Get test options from command line */
        for (;;) {
                c = getopt(argc, argv, "t:c:p:i:d:h");
                if (c == -1) {
                        break;
                }

                if (c == 'h') {
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
        param->priority =
                options->priority >= index + 1 ? options->priority - index : 1;
        param->interval = options->interval + options->distance * index;
        param->affinity = index % options->cpu_num;
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

        printf("T: %2u P: %2u A: %2u I: %5" PRIu64 " C: %7" PRIu64
               " Min: %5" PRIu64 " Act: %5" PRIu64 " Avg: %5" PRIu64
               " Max: %5" PRIu64 "\n",
               index,
               param->priority,
               param->affinity,
               param->interval,
               stat->cycles,
               stat->min,
               stat->act,
               (u64)stat->avg,
               stat->max);
}

static void *thread(void *arg)
{
        u64 latency;
        double sum;
        struct timespec before, after, interval;
        struct thread_param *param = arg;
        struct thread_stat *stat = param->stat;

        set_affinity(param->affinity);

        timespec_from_usec(&interval, param->interval);

        for (;;) {
                clock_gettime(CLOCK_REALTIME, &before);
                clock_nanosleep(CLOCK_REALTIME, 0, &interval, NULL);
                clock_gettime(CLOCK_REALTIME, &after);
                latency = timespec_diff(&after, &before) - param->interval;

                stat->act = latency;
                stat->min = MIN(stat->min, latency);
                stat->max = MAX(stat->max, latency);
                sum = stat->avg * stat->cycles + latency;
                stat->cycles++;
                stat->avg = sum / stat->cycles;
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
                create_thread(thread, &params[i], params[i].priority);
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
