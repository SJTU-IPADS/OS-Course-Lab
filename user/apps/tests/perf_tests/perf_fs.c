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
#include "unistd.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sched.h>
#include <stdint.h>
#include <sys/stat.h>
#include "perf_tools.h"

#define DIR_PREFIX   "/test"
#define TEST_FILENUM PERF_NUM_SMALL

int main()
{
        int i, ret;
        char filename[256];
        char *buf;
        int fds[TEST_FILENUM];
        perf_time_t *start_perf_time = malloc(sizeof(perf_time_t));
        perf_time_t *end_perf_time = malloc(sizeof(perf_time_t));
        if (bind_thread_to_core(2) < 0)
                error("sched_setaffinity failed!\n");
        mkdir(DIR_PREFIX, S_IRUSR | S_IWUSR | S_IXUSR);

        /* measure create latency */
        record_time(start_perf_time);
        for (i = 0; i < TEST_FILENUM; i++) {
                sprintf(filename, "./test/test-%d.txt", i);
                ret = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                assert(ret > 0);
                fds[i] = ret;
        }
        record_time(end_perf_time);
        printf("create file latency result:\n");
        print_perf_result(start_perf_time, end_perf_time, TEST_FILENUM);

        buf = calloc(512, sizeof(uint64_t));
        /* measure write latency */
        record_time(start_perf_time);
        for (i = 0; i < TEST_FILENUM; i++) {
                ret = write(fds[i], buf, 4096);
                assert(ret == 4096);
        }
        record_time(end_perf_time);
        printf("write file latency(4KB) result:\n");
        print_perf_result(start_perf_time, end_perf_time, TEST_FILENUM);

        /* rewind */
        for (i = 0; i < TEST_FILENUM; i++) {
                ret = lseek(fds[i], 0, SEEK_SET);
                assert(ret == 0);
        }

        /* measure read latency */
        record_time(start_perf_time);
        for (i = 0; i < TEST_FILENUM; i++) {
                ret = read(fds[i], buf, 4096);
                assert(ret == 4096);
        }
        record_time(end_perf_time);
        printf("read file latency(4KB) result:\n");
        print_perf_result(start_perf_time, end_perf_time, TEST_FILENUM);
#if defined(FS_TEST_LARGE_FILE)
        buf = calloc(512 * 512, sizeof(uint64_t));

        /* rewind */
        for (i = 0; i < TEST_FILENUM; i++) {
                lseek(fds[i], 0, SEEK_SET);
        }

        /* measure write latency */
        record_time(start_perf_time);
        for (i = 0; i < TEST_FILENUM; i++) {
                ret = write(fds[i], buf, 4096 * 512);
                assert(ret == 4096 * 512);
        }
        record_time(end_perf_time);
        printf("write file latency(2MB) result:\n");
        print_perf_result(start_perf_time, end_perf_time, TEST_FILENUM);

        /* rewind */
        for (i = 0; i < TEST_FILENUM; i++) {
                lseek(fds[i], 0, SEEK_SET);
        }

        /* measure read latency */
        record_time(start_perf_time);
        for (i = 0; i < TEST_FILENUM; i++) {
                ret = read(fds[i], buf, 4096 * 512);
        }
        record_time(end_perf_time);
        printf("read file latency(2MB) result:\n");
        print_perf_result(start_perf_time, end_perf_time, TEST_FILENUM);
#endif
        for (i = 0; i < TEST_FILENUM; i++) {
                ret = close(fds[i]);
                assert(ret == 0);
        }

        for (i = 0; i < TEST_FILENUM; i++) {
                sprintf(filename, "./test/test-%d.txt", i);
                ret = unlink(filename);
                assert(ret == 0);
        }
        unlink(DIR_PREFIX);

        free(start_perf_time);
        free(end_perf_time);
}