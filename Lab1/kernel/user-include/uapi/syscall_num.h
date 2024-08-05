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

#ifndef UAPI_SYSCALL_NUM_H
#define UAPI_SYSCALL_NUM_H

#define NR_SYSCALL 64

/* Character IO */
#define CHCORE_SYS_putstr 0
#define CHCORE_SYS_getc   1

/* PMO */
#define CHCORE_SYS_create_pmo        2
#define CHCORE_SYS_map_pmo           4
#define CHCORE_SYS_unmap_pmo         5
#define CHCORE_SYS_write_pmo         6
#define CHCORE_SYS_read_pmo          7
#define CHCORE_SYS_get_phys_addr     8

/* Capability */
#define CHCORE_SYS_revoke_cap        9
#define CHCORE_SYS_transfer_caps    10

/* Multitask */
/* - create & exit */
#define CHCORE_SYS_create_cap_group 11
#define CHCORE_SYS_exit_group       12
#define CHCORE_SYS_kill_group       60
#define CHCORE_SYS_create_thread    13
#define CHCORE_SYS_thread_exit      14
/* - recycle */
#define CHCORE_SYS_register_recycle     15
#define CHCORE_SYS_cap_group_recycle    16
#define CHCORE_SYS_ipc_close_connection 17
/* - schedule */
#define CHCORE_SYS_yield        18
#define CHCORE_SYS_set_affinity 19
#define CHCORE_SYS_get_affinity 20
#define CHCORE_SYS_set_prio     21
#define CHCORE_SYS_get_prio     22
#define CHCORE_SYS_suspend      23
#define CHCORE_SYS_resume       24
/* - ptrace */
#define CHCORE_SYS_ptrace       25

/* IPC */
#define CHCORE_SYS_register_server         26
#define CHCORE_SYS_register_client         27
#define CHCORE_SYS_ipc_register_cb_return  28
#define CHCORE_SYS_ipc_call                29
#define CHCORE_SYS_ipc_return              30
#define CHCORE_SYS_ipc_exit_routine_return 31
#define CHCORE_SYS_ipc_get_cap             32
#define CHCORE_SYS_ipc_set_cap             33
/* - notification */
#define CHCORE_SYS_create_notifc           34
#define CHCORE_SYS_wait                    35
#define CHCORE_SYS_notify                  36

/* Exception */
/* - irq */
#define CHCORE_SYS_irq_register            37
#define CHCORE_SYS_irq_wait                38
#define CHCORE_SYS_irq_ack                 39
#define CHCORE_SYS_configure_irq           40
/* - page fault */
#define CHCORE_SYS_user_fault_register     41
#define CHCORE_SYS_user_fault_map          42

/* POSIX */
/* - time */
#define CHCORE_SYS_clock_gettime           43
#define CHCORE_SYS_clock_nanosleep         44
/* - memory */
#define CHCORE_SYS_handle_brk              45
#define CHCORE_SYS_handle_mprotect         46

/* Hardware Access */
/* - cache */
#define CHCORE_SYS_cache_flush             47
#define CHCORE_SYS_cache_config            48
/* - timer */
#define CHCORE_SYS_get_current_tick        49
/* Get PCI devcie information. */
#define CHCORE_SYS_get_pci_device          50
/* poweroff */
#define CHCORE_SYS_poweroff                51

/* Utils */
#define CHCORE_SYS_empty_syscall           52
#define CHCORE_SYS_top                     53
#define CHCORE_SYS_get_free_mem_size       54
#define CHCORE_SYS_get_mem_usage_msg       55
#define CHCORE_SYS_get_system_info         56

/* - futex */
#define CHCORE_SYS_futex      57
#define CHCORE_SYS_set_tid_address  58

#endif /* UAPI_SYSCALL_NUM_H */
