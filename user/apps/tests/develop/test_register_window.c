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
#include <bits/trap.h>

#include "test_tools.h"

#define DEFAULT_RECURSIVE_DEPTH 20

void user_window_test(int depth)
{
        if (depth == 0)
                return;
        user_window_test(--depth);
}

int main(int argc, char *argv[])
{
        int depth = DEFAULT_RECURSIVE_DEPTH;

        if (argc >= 2) {
                depth = atoi(argv[1]);
                if (depth <= 0) {
                        error("Depth should be positive!\n");
                        exit(1);
                }
        }

        info("Test register window begins...\n");
        user_window_test(depth);
        info("User register window test finished!\n");

        asm volatile("ta %0\n" ::"i"(WINDOW_TEST_TRAP));
        info("Kernel register window test finished!\n");

        green_info("Test register window finished!");

        return 0;
}
