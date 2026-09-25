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

#ifndef SRVMGR_H
#define SRVMGR_H

#ifdef CHCORE_OPENTRUSTEE
#include <spawn_ext.h>
#endif /* CHCORE_OPENTRUSTEE */
#include <chcore/ipc.h>
#include <chcore-internal/procmgr_defs.h>
#include "proc_node.h"

struct new_process_args {
        int envp_argc;
        char **envp_argv;
        unsigned long stack_size;
#ifdef CHCORE_OPENTRUSTEE
        spawn_uuid_t puuid;
        unsigned long heap_size;
#endif /* CHCORE_OPENTRUSTEE */
};

/* fsm_server_cap in current process; can be copied to others */
extern cap_t fsm_server_cap;
/* lwip_server_cap in current process; can be copied to others */
extern cap_t lwip_server_cap;
/* in srvmgr.c */
extern cap_t __procmgr_server_cap;

extern char __binary_fsm_elf_start;
extern char __binary_fsm_elf_size;

extern char __binary_tmpfs_elf_start;
extern char __binary_tmpfs_elf_size;

int procmgr_launch_process(int input_argc, char **input_argv, char *name,
                           bool if_has_parent, badge_t parent_badge,
                           struct new_process_args *np_args,
                           int proc_type, struct proc_node **out_proc);
int procmgr_launch_basic_server(int input_argc, char **input_argv, char *name,
                                bool if_has_parent, badge_t parent_badge,
                                struct proc_node **out_proc);

/*
 * There three kinds of servers in ChCore for now.
 *  1. servers booted before procmgr like tmpfs, fsm and lwip
 *  2. servers which will be booted once procmgr is up like usb_devmgr
 *  3. servers which will be booted lazily like shmmgr, hdmi, sd4, etc.
 *
 * boot_secondary_servers() is in charge of booting the second kind of servers.
 */
void set_tmpfs_cap(cap_t cap);
void boot_secondary_servers(void);

/* Initialize sys_servers and sys_server_locks */
void init_srvmgr(void);
void handle_get_service_cap(ipc_msg_t *ipc_msg, struct proc_request *pr);
void handle_set_service_cap(ipc_msg_t *ipc_msg, badge_t badge, struct proc_request *pr);
void start_daemon_service(void);
#endif /* SRVMGR_H */
