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
#include <time.h>
#include <inttypes.h>

#include "test_tools.h"

#define TEST_NUM 200

int main(int argc, char *argv[])
{
        int ret;
        struct timespec *test[TEST_NUM];

        info("Test clockgettime begins...\n");

        for (int i = 0; i < TEST_NUM; ++i) {
                test[i] = (struct timespec *)malloc(sizeof(struct timespec));
                memset(test[i], 0, sizeof(struct timespec));
        }

        for (int i = 0; i < TEST_NUM; ++i) {
                ret = clock_gettime(0, test[i]);
                chcore_assert(ret == 0, "clock_gettime failed!");
                info("Get time: %" PRId64 " s, %ldns\n",
                        test[i]->tv_sec, test[i]->tv_nsec);
        }

        green_info("Test clockgettime finished!\n");

        return 0;
}
