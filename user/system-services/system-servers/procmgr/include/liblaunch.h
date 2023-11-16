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

/**
 * @file liblaunch.h
 *
 * @brief This library is used to launch a process in ChCore,
 * whose memory image is given by an ELF file.
 */
#ifndef LIBLAUNCH_H
#define LIBLAUNCH_H

#include <chcore/type.h>
#include "libchcoreelf.h"

#ifdef __cplusplus
extern "C" {
#endif

struct cap_group_args {
        badge_t badge;
        vaddr_t name;
        unsigned long name_len;
        unsigned long pcid;
};

struct launch_process_args {
        /** input parameters */
        /**
         * user_elf struct of the new process.
         * This struct represents the memory image of the new process,
         * or to be executed program.
         */
        struct user_elf *user_elf;
        /**
         * It non-zero, then the user_elf would be mapped starting at
         * load_offset instead of 0 in the new process's address space.
         */
        vaddr_t load_offset;
        /**
         * Custom PMOs (except for the PMOs in the user_elf) to be mapped
         * into the new process's address space.
         */
        struct pmo_map_request *pmo_map_reqs;
        int nr_pmo_map_reqs;
        /**
         * Custom capabilities to be transfered to the new process. We would
         * transfer caps of 3 fundamental system server, i.e procmgr, fsm and
         * lwip, to the new process by default.
         */
        cap_t *caps;
        int nr_caps;
        /**
         * CPU affinity of the main thread of the new process.
         */
        s32 cpuid;
        int argc;
        /**
         * NULL-termiated array of arguments to be passed to the new process. It
         * should contain @argc valid parameters, otherwise the behavior is
         * undefined.
         */
        char **argv;
        /**
         * Unique badge used to identify different processes by the ChCore
         * kernel.
         */
        badge_t badge;
        int pid;
        /**
         * Address space ID for the new process, it should be unique to reduce
         * TLB flush when switching between two processes if possible.
         */
        u64 pcid;
        /**
         * Name of the new process, which would be stored in the kernel for
         * debugging.
         */
        char *process_name;
        /**
         * Whether the new process is going to be traced (i.e. by a gdb server).
         * If set to true, the main thread of the new process will be suspended
         * as soon as created.
         */
        bool is_traced;

        int envp_argc;
        char **envp_argv;
        unsigned long stack_size;

        /** output parameters */
        cap_t *child_process_cap;
        cap_t *child_main_thread_cap;
};

/*
 * @brief Launch a new process in ChCore. The new process would execute
 * programs in the given ELF file.
 *
 * @param lp_args [In] see documentation of struct launch_process_args.
 * @return 0 on success, otherwise an error code. All memory resources
 * allocated by this function would be freed on error.
 */
int launch_process_with_pmos_caps(struct launch_process_args *lp_args);

#ifdef __cplusplus
}
#endif

#endif /* LIBLAUCH_H */