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

#include <chcore/syscall.h>
#include <chcore/defs.h>
#include <chcore/ipc.h>
#include <stdio.h>
#include <string.h>
#include "perf_tools.h"

DEFINE_SERVER_HANDLER(server_trampoline)
{
#if defined(CHCORE_ARCH_X86_64)
        asm volatile("syscall" : : "a"(CHCORE_SYS_ipc_return), "D"(0), "S"(0) :);
#else
        ipc_return(NULL, 0);
#endif
}

int main(int argc, char *argv[])
{
        void *token;

        if (argc == 1)
                goto out;

        token = strstr(argv[1], "TOKEN");
        if (!token)
                goto out;

        ipc_register_server(server_trampoline, DEFAULT_CLIENT_REGISTER_HANDLER);

        usys_exit(0);
        return 0;
out:
        error("[IPC Server] no clients. Bye!\n");
        return 0;
}
