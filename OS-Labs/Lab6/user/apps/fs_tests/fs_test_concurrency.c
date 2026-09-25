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

#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <chcore/syscall.h>
#include <chcore/proc.h>
#include <chcore/launcher.h>
#include <sys/mount.h>

#define FS_TEST_CC_DEGREE 5
#define TMPFS_TEST_ONLY   1

int main()
{
        int start_time = clock();
#if TMPFS_TEST_ONLY
        char *test_process[2];
        struct new_process_caps client_process_caps[FS_TEST_CC_DEGREE];
        pid_t test_pids[FS_TEST_CC_DEGREE];
        int i;
        int status;

        /* Create some client processes */
        test_process[0] = "fs_test_full.bin";
        test_process[1] = malloc(50);
        strcpy(test_process[1], "/base_sub_a");
        for (i = 0; i < FS_TEST_CC_DEGREE; i++) {
                test_process[1][strlen(test_process[1]) - 1] = 'a' + i;
                test_pids[i] = create_process(
                        2, test_process, &client_process_caps[i]);
        }

        /* Wait child processes */
        for (i = 0; i < FS_TEST_CC_DEGREE; i++)
                chcore_waitpid(
                        test_pids[i], &status, /* dummy */ 0, /* dummy */ 0);
#else
        char *test_process[2];
        struct new_process_caps client_process_caps[FS_TEST_CC_DEGREE][3];
        pid_t test_pids[FS_TEST_CC_DEGREE][3];
        int i, j;
        int status;

        /* Mount */
        mount("sda1", "/fstest_fat", 0, 0, 0);
        mount("sda2", "/fstest_ext", 0, 0, 0);

        /* Create some client processes */
        test_process[0] = "fs_test_full.bin";
        test_process[1] = malloc(50);
        for (i = 0; i < FS_TEST_CC_DEGREE; i++) {
                for (j = 0; j < 3; j++) {
                        if (j == 0)
                                strcpy(test_process[1], "");
                        if (j == 1)
                                strcpy(test_process[1], "/fstest_fat");
                        if (j == 2)
                                strcpy(test_process[1], "/fstest_ext");
                        strcat(test_process[1], "/base_sub_a");
                        test_process[1][strlen(test_process[1]) - 1] = 'a' + i;
                        test_pids[i][j] = create_process(
                                2, test_process, &client_process_caps[i][j]);
                }
        }

        /* Wait child processes */
        for (i = 0; i < FS_TEST_CC_DEGREE; i++)
                for (j = 0; j < 3; j++)
                        chcore_waitpid(test_pids[i][j],
                                       &status,
                                       /* dummy */ 0,
                                       /* dummy */ 0);
#endif
        int end_time = clock();
        double total_time = ((double)(end_time - start_time)) / 1000000;
        printf("Time Consumption: %.2lfs\n", total_time);

        printf("test_fs_concurrency finished\n");
        return 0;
}