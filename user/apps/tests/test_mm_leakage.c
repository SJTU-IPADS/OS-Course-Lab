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
#include <chcore/proc.h>
#include <chcore/launcher.h>
#include <chcore/syscall.h>

#include "test_tools.h"

int main(int argc, char *argv[])
{
        unsigned long size1, size2;
        int exit_status, ret;
        pid_t pid;
        char *args[1];
        struct free_mem_info free_mem_info;

        /*
         * Note: This is a program for testing if there is any memory leakage
         * when running other user apps. This program only condisers current
         * situation and the accuracy of the future cannot be guaranteed.
         * Please think deeply about the testing result as there are many
         * factors that may influence it.
         *
         * Before run this program, you should:
         * 1. Move the statement which changes the state of the exited process
         *    to PROC_STATE_EXIT after the process usys_cap_group_recycle.
         *    See details in user/procmgr/recycle.c #recycle_routine().
         * 2. Uncomment the statement which can count free memory size in
         *    /user/procmgr/procmgr.c #handle_newproc().
         * 3. The memory size counted before launch_process_with_pmos_caps
         *    and after recycling are expected to be equal.
         */

        /* The second argument is the file name which you want to test. */
        if (argc < 2) {
                info("Usage: %s [test file]\n", argv[0]);
                return 0;
        }

        info("Test memory leakage begins...\n");

        args[0] = "/test_hello.bin";
        pid = chcore_new_process(1, args);
        chcore_assert(pid >= 0, "create process failed");
        chcore_waitpid(pid, &exit_status, 0, 0);

        /* Get the size of free memory before creating a new app */
        ret = usys_get_free_mem_size(&free_mem_info);
        chcore_assert(ret == 0, "get free mem size failed");
        
        size1 = free_mem_info.free_mem_size;
        info("Free memory size before create %s: %#lx\n",
                argv[1], size1);

        /* Send a ipc request to prcmgr to create a new process */
        pid = chcore_new_process(argc - 1, &argv[1]);
        chcore_assert(pid >= 0, "create process failed");

        /* Block until the process created above exits */
        chcore_waitpid(pid, &exit_status, 0, 0);

        /* Get the size of free memory again after the process was recycled */
        ret = usys_get_free_mem_size(&free_mem_info);
        chcore_assert(ret == 0, "get free mem size failed");

        size2 = free_mem_info.free_mem_size;
        info("Free memory size after recycle: %#lx\n", size2);
        chcore_assert(size1 == size2, "Free memory not equal!");

        green_info("Test memory leakage finished\n");

        return 0;
}
