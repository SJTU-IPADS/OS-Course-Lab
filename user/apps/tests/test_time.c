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
#include <sys/time.h>
#include <time.h>
#include <inttypes.h>

#include "test_tools.h"

int main(int argc, char *argv[])
{
       int ret;
       struct timespec t;
       struct timeval tv;

       info("Test time begins...\n");

       ret = clock_gettime(0, &t);
       chcore_assert(ret == 0, "clock_gettime failed!");
       info("clock_gettime: %" PRId64 " seconds, %ld nanoseconds\n",
            t.tv_sec, t.tv_nsec);

       ret = gettimeofday(&tv, NULL);
       chcore_assert(ret == 0, "gettimeofday failed!");
       info("gettimeofday: %" PRId64 " seconds, %" PRId64 " milliseconds\n",
            tv.tv_sec, tv.tv_usec);

       green_info("Test time finished\n");

       return 0;
}
