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
#include <assert.h>

int main()
{
        pid_t pid;
        char *args[1];
        int exit_status;

        args[0] = "/test_hello.bin";
        pid = chcore_new_process(1, args);
        assert(pid >= 0);
        printf("chcore_new_process success.\n");
        pid = chcore_waitpid(pid, &exit_status, 0, 0);
        assert(pid >= 0);
        printf("chcore_waitpid success.\n");
        printf("Test finish!\n");
        return 0;
}
