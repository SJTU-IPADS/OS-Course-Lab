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

#include <chcore/syscall.h>
#include <stdio.h>
#include <chcore/type.h>
#include <chcore/defs.h>
#include <malloc.h>
#include <chcore/ipc.h>
#include <chcore/proc.h>
#include <chcore/launcher.h>
#include <sched.h>
#include <inttypes.h>
#include "perf_tools.h"

#define IPC_NUM PERF_NUM_LARGE

void perf_ipc_routine(int server_cap)
{
        ipc_struct_t *client_ipc_struct;
        ipc_msg_t *ipc_msg;
        int i;

        /* register IPC client */
        client_ipc_struct = ipc_register_client(server_cap);
        if (client_ipc_struct == NULL) {
                error("ipc_register_client failed\n");
                usys_exit(-1);
        }

        perf_time_t *start_perf_time = malloc(sizeof(perf_time_t));
        perf_time_t *end_perf_time = malloc(sizeof(perf_time_t));

        ipc_msg = ipc_create_msg(client_ipc_struct, 0);

        /* warm up */
        for (i = 0; i < 1000; i++) {
                ipc_call(client_ipc_struct, ipc_msg);
        }

        register unsigned long ipc_conn_cap = client_ipc_struct->conn_cap;
#if defined(CHCORE_ARCH_X86_64)
        register cycle_t ret;

        /* warm up */
        for (i = 0; i < 10000; i++) {
                asm volatile(
                        "syscall"
                        : "=a"(ret)
                        : "a"(CHCORE_SYS_ipc_call), "D"(ipc_conn_cap), "S"(0)
                        : "rcx", "r11", "memory");
        }

        record_time(start_perf_time);
        for (i = 0; i < IPC_NUM; i++) {
                asm volatile(
                        "syscall"
                        : "=a"(ret)
                        : "a"(CHCORE_SYS_ipc_call), "D"(ipc_conn_cap), "S"(0)
                        : "rcx", "r11", "memory");
        }
        record_time(end_perf_time);
#elif defined(CHCORE_ARCH_AARCH64)
        record_time(start_perf_time);
        for (i = 0; i < IPC_NUM; i++) {
                register long x8 __asm__("x8") = CHCORE_SYS_ipc_call;
                register long x0 __asm__("x0") = ipc_conn_cap;
                register long x1 __asm__("x1") = 0;
                asm volatile("svc 0"
                             : "=r"(x0)
                             : "r"(x8), "0"(x0), "r"(x1)
                             : "cc", "memory");
        }
        record_time(end_perf_time);
#elif defined(CHCORE_ARCH_RISCV64)
        record_time(start_perf_time);
        for (i = 0; i < IPC_NUM; i++) {
                register long a7 __asm__("a7") = CHCORE_SYS_ipc_call;
                register long a0 __asm__("a0") = ipc_conn_cap;
                register long a1 __asm__("a1") = 0;

                asm volatile("ecall"
                             : "=r"(a0)
                             : "r"(a7), "0"(a0), "r"(a1)
                             : "memory");
        }
        record_time(end_perf_time);
#elif defined(CHCORE_ARCH_SPARC)
        record_time(start_perf_time);
        for (i = 0; i < IPC_NUM; i++) {
                register long g1 __asm__("g1") = CHCORE_SYS_ipc_call;
                register long o0 __asm__("o0") = ipc_conn_cap;
                register long o1 __asm__("o1") = 0;

                asm volatile("ta 0x10"
                             : "+r"(o0)
                             : "r"(g1), "0"(o0), "r"(o1)
                             : "memory");
        }
        record_time(end_perf_time);
#else
#error "Unsupported architecture"
#endif
        info("IPC Result:\n");
        print_perf_result(start_perf_time, end_perf_time, IPC_NUM);

        ipc_destroy_msg(ipc_msg);
        free(start_perf_time);
        free(end_perf_time);
}

int main(int argc, char *argv[], char *envp[])
{
        char *args[2];
        struct new_process_caps new_process_caps;
        cap_t server_cap;

#if defined(CHCORE_ARCH_X86_64)
        printf("Perf IPC on x86_64\n");
#elif defined(CHCORE_ARCH_AARCH64)
        printf("Perf IPC on aarch64\n");
#elif defined(CHCORE_ARCH_RISCV64)
        printf("Perf IPC on riscv64\n");
#elif defined(CHCORE_ARCH_SPARC)
        printf("Perf IPC on SPARC\n");
#else
#error "Unsupported architecture"
#endif

        if (bind_thread_to_core(2) < 0)
                error("sched_setaffinity failed!\n");

        args[0] = "/perf_ipc_server.bin";
        args[1] = "TOKEN";
        create_process(2, args, &new_process_caps);
        server_cap = new_process_caps.mt_cap;

        info("Start measure IPC\n");

        /* Perf test start */
        perf_ipc_routine(server_cap);

        // TODO: how to shutdown the server process
        // server_exit_flag = 1;

        return 0;
}
