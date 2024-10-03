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

#pragma once

#include <chcore/defs.h>
#include <chcore/type.h>
#include <stdio.h>
#include <chcore/memory.h>
#include <uapi/object.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

void usys_putstr(vaddr_t buffer, size_t size);
char usys_getc(void);

cap_t usys_create_device_pmo(unsigned long paddr, unsigned long size);
cap_t usys_create_pmo_with_rights(unsigned long size, unsigned long type, cap_right_t rights);
cap_t usys_create_pmo(unsigned long size, unsigned long type);
cap_t usys_create_pmo_with_val(unsigned long size, unsigned long type, unsigned long val);
int usys_map_pmo(cap_t cap_group_cap, cap_t pmo_cap,
                 unsigned long addr, unsigned long perm);
int usys_unmap_pmo(cap_t cap_group_cap, cap_t pmo_cap,
                   unsigned long addr);
int usys_write_pmo(cap_t pmo_cap, unsigned long offset,
                   void *buf, unsigned long size);
int usys_read_pmo(cap_t cap, unsigned long offset, void *buf,
                  unsigned long size);
int usys_get_phys_addr(void *vaddr, unsigned long *paddr);

int usys_revoke_cap(cap_t obj_cap, bool revoke_copy);
int usys_transfer_caps(cap_t dest_group_cap, cap_t *src_caps, int nr_caps,
                       cap_t *dst_caps);
int usys_transfer_caps_restrict(cap_t dest_group_cap, cap_t *src_caps,
                                int nr_caps, cap_t *dst_caps,
                                cap_right_t *masks, cap_right_t *rests);

_Noreturn void usys_exit(unsigned long ret);
void usys_yield(void);
int usys_suspend(unsigned long thread_cap);
int usys_resume(unsigned long thread_cap);

cap_t usys_create_thread(unsigned long thread_args_p);
cap_t usys_create_cap_group(unsigned long cap_group_args_p);
int usys_kill_group(int proc_cap);
int usys_register_server(unsigned long ipc_handler,
                                   cap_t reigster_cb_cap,
                                   unsigned long destructor);
cap_t usys_register_client(cap_t server_cap, unsigned long vm_config_ptr);
long usys_ipc_call(cap_t conn_cap, unsigned int cap_num);
_Noreturn void usys_ipc_return(unsigned long ret, unsigned long cap_num);
void usys_ipc_register_cb_return(cap_t server_thread_cap,
                                 unsigned long server_thread_exit_routine,
                                 unsigned long server_shm_addr);
_Noreturn void usys_ipc_exit_routine_return(void);
cap_t usys_ipc_get_cap(cap_t conn, int index);
int usys_ipc_set_cap(cap_t conn, int index, cap_t cap);
int usys_ipc_set_cap_restrict(cap_t conn, int index, cap_t cap,
                              cap_right_t mask, cap_right_t rest);

int usys_set_affinity(cap_t thread_cap, s32 aff);
s32 usys_get_affinity(cap_t thread_cap);
int usys_set_prio(cap_t thread_cap, int prio);
int usys_get_prio(cap_t thread_cap);

int usys_get_free_mem_size(struct free_mem_info *info);
void usys_get_mem_usage_msg(void);
void usys_empty_syscall(void);
void usys_top(void);

int usys_user_fault_register(cap_t notific_cap, vaddr_t msg_buffer);
int usys_user_fault_map(badge_t client_badge, vaddr_t fault_va,
                        vaddr_t remap_va, bool copy, vmr_prop_t perm);
int usys_map_pmo_with_length(cap_t pmo_cap, vaddr_t addr, unsigned long perm,
                             size_t length);
int usys_unmap_pmo_with_length(cap_t pmo_cap, unsigned long addr, size_t size);

cap_t usys_irq_register(int irq);
int usys_irq_wait(cap_t irq_cap, bool is_block);
int usys_irq_ack(cap_t irq_cap);

#ifdef CHCORE_ARCH_SPARC
void usys_disable_irq(void);
void usys_enable_irq(void);
void usys_enable_irqno(unsigned long irqno);
void usys_disable_irqno(unsigned long irqno);
void usys_cache_config(unsigned long option);
#endif

cap_t usys_create_notifc(void);
int usys_wait(cap_t notifc_cap, bool is_block, struct timespec *timeout);
int usys_notify(cap_t notifc_cap);

int usys_register_recycle_thread(cap_t cap, unsigned long buffer);
int usys_ipc_close_connection(cap_t cap);

int usys_cache_flush(unsigned long start, unsigned long size, int op_type);
unsigned long usys_get_current_tick(void);

void usys_get_pci_device(int dev_class, unsigned long uaddr);

void usys_poweroff(void);

int usys_get_system_info(int op, void *ubuffer,
                         unsigned long size, long arg);

int usys_ptrace(int req, unsigned long cap, unsigned long tid,
		void *addr, void *data);

long usys_opentrustee(long z, long a, long b, long c, long d, long e, long f);

#ifdef __cplusplus
}
#endif
