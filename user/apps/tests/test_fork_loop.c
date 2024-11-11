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

#include <chcore/launcher.h>
#include <chcore/proc.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_tools.h"

#define LOOP_TIMES      1000
#define TEST_PROC_NAME  "/test_hello.bin"

int main(int argc, char *argv[], char *envp[])
{
        struct new_process_caps new_process_caps;
        int loop_times = LOOP_TIMES, pid, status;

        if (argc >= 2) {
                loop_times = atoi(argv[1]);
        }
        info("Test fork loop begins...\n");
        info("Create process %s, loop %d times\n",
             TEST_PROC_NAME, loop_times);

        for (int i = 0; i < loop_times; ++i) {
                argv[0] = TEST_PROC_NAME;
                pid = create_process(1, argv, &new_process_caps);
                chcore_assert(pid >= 0, "create process failed!(%d)", i);
                chcore_waitpid(pid, &status, 0, 0);
        }

        green_info("Test fork loop finished!\n");

        return 0;
}
