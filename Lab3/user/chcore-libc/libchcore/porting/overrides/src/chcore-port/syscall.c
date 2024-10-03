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
#include <chcore/type.h>
#include <chcore/proc.h>
#include <syscall_arch.h>
#include <raw_syscall.h>
#include <errno.h>

#include <stdio.h>

void usys_putstr(vaddr_t buffer, size_t size)
{
        chcore_syscall2(CHCORE_SYS_putstr, buffer, size);
}

char usys_getc(void)
{
        return chcore_syscall0(CHCORE_SYS_getc);
}

_Noreturn void usys_exit(unsigned long ret)
{
        chcore_syscall1(CHCORE_SYS_thread_exit, ret);
        __builtin_unreachable();
}

void usys_yield(void)
{
        chcore_syscall0(CHCORE_SYS_yield);
}

int usys_suspend(unsigned long thread_cap)
{
        return chcore_syscall1(CHCORE_SYS_suspend, thread_cap);
}

int usys_resume(unsigned long thread_cap)
{
        return chcore_syscall1(CHCORE_SYS_resume, thread_cap);
}

cap_t usys_create_device_pmo(unsigned long paddr, unsigned long size)
{
        return chcore_syscall4(CHCORE_SYS_create_pmo, size, PMO_DEVICE, paddr, PMO_ALL_RIGHTS);
}

cap_t usys_create_pmo_with_rights(unsigned long size, unsigned long type, cap_right_t rights)
{
        return chcore_syscall4(CHCORE_SYS_create_pmo, size, type, 0, rights);
}

cap_t usys_create_pmo(unsigned long size, unsigned long type)
{
        return chcore_syscall4(CHCORE_SYS_create_pmo, size, type, 0, PMO_ALL_RIGHTS);
}

cap_t usys_create_pmo_with_val(unsigned long size, unsigned long type, unsigned long val)
{
        return chcore_syscall4(CHCORE_SYS_create_pmo, size, type, val, PMO_ALL_RIGHTS);
}

int usys_map_pmo(cap_t cap_group_cap, cap_t pmo_cap,
                 unsigned long addr, unsigned long rights)
{
        return chcore_syscall5(CHCORE_SYS_map_pmo,
                               cap_group_cap,
                               pmo_cap,
                               addr,
                               rights,
                               -1 /* pmo size */);
}

int usys_unmap_pmo(cap_t cap_group_cap, cap_t pmo_cap, unsigned long addr)
{
        return chcore_syscall4(
                CHCORE_SYS_unmap_pmo, cap_group_cap, pmo_cap, addr, -1);
}

int usys_revoke_cap(cap_t obj_cap, bool revoke_copy)
{
        return chcore_syscall2(CHCORE_SYS_revoke_cap, obj_cap, revoke_copy);
}

int usys_set_affinity(cap_t thread_cap, s32 aff)
{
        return chcore_syscall2(
                CHCORE_SYS_set_affinity, thread_cap, (unsigned long)aff);
}

s32 usys_get_affinity(cap_t thread_cap)
{
        return chcore_syscall1(CHCORE_SYS_get_affinity, thread_cap);
}

int usys_set_prio(cap_t thread_cap, int prio)
{
        return chcore_syscall2(
                CHCORE_SYS_set_prio, thread_cap, (unsigned long)prio);
}

int usys_get_prio(cap_t thread_cap)
{
        return chcore_syscall1(CHCORE_SYS_get_prio, thread_cap);
}

int usys_get_phys_addr(void *vaddr, unsigned long *paddr)
{
        return chcore_syscall2(CHCORE_SYS_get_phys_addr,
                               (unsigned long)vaddr,
                               (unsigned long)paddr);
}

cap_t usys_create_thread(unsigned long thread_args_p)
{
        return chcore_syscall1(CHCORE_SYS_create_thread, thread_args_p);
}

cap_t usys_create_cap_group(unsigned long cap_group_args_p)
{
        return chcore_syscall1(CHCORE_SYS_create_cap_group, cap_group_args_p);
}

int usys_kill_group(int proc_cap)
{
	return chcore_syscall1(CHCORE_SYS_kill_group, proc_cap);
}

int usys_register_server(unsigned long callback,
                         cap_t register_thread_cap,
                         unsigned long destructor)
{
        return chcore_syscall3(CHCORE_SYS_register_server,
                               callback,
                               register_thread_cap,
                               destructor);
}

cap_t usys_register_client(cap_t server_cap, unsigned long vm_config_ptr)
{
        return chcore_syscall2(
                CHCORE_SYS_register_client, server_cap, vm_config_ptr);
}

long usys_ipc_call(cap_t conn_cap, unsigned int cap_num)
{
        return chcore_syscall2(
                CHCORE_SYS_ipc_call, conn_cap, cap_num);
}

_Noreturn void usys_ipc_return(unsigned long ret, unsigned long cap_num)
{
        chcore_syscall2(CHCORE_SYS_ipc_return, ret, cap_num);
        __builtin_unreachable();
}

void usys_ipc_register_cb_return(cap_t server_thread_cap,
                                 unsigned long server_thread_exit_routine,
                                 unsigned long server_shm_addr)
{
        chcore_syscall3(CHCORE_SYS_ipc_register_cb_return,
                        server_thread_cap,
                        server_thread_exit_routine,
                        server_shm_addr);
}

_Noreturn void usys_ipc_exit_routine_return(void)
{
        chcore_syscall0(CHCORE_SYS_ipc_exit_routine_return);
        __builtin_unreachable();
}

cap_t usys_ipc_get_cap(cap_t conn, int index)
{
        return chcore_syscall2(CHCORE_SYS_ipc_get_cap, conn, index);
}

int usys_ipc_set_cap_restrict(cap_t conn, int index, cap_t cap,
                              cap_right_t mask, cap_right_t rest)
{
        return chcore_syscall5(CHCORE_SYS_ipc_set_cap,
                               conn,
                               index,
                               cap,
                               mask,
                               rest);
}

int usys_ipc_set_cap(cap_t conn, int index, cap_t cap)
{
        return usys_ipc_set_cap_restrict(conn,
                                         index,
                                         cap,
                                         CAP_RIGHT_NO_RIGHTS,
                                         CAP_RIGHT_NO_RIGHTS);
}

int usys_write_pmo(cap_t cap, unsigned long offset, void *buf,
                   unsigned long size)
{
        return chcore_syscall4(
                CHCORE_SYS_write_pmo, cap, offset, (unsigned long)buf, size);
}

int usys_read_pmo(cap_t cap, unsigned long offset, void *buf,
                  unsigned long size)
{
        return chcore_syscall4(
                CHCORE_SYS_read_pmo, cap, offset, (unsigned long)buf, size);
}

int usys_transfer_caps(cap_t cap_group, cap_t *src_caps, int nr,
                       cap_t *dst_caps)
{
        return usys_transfer_caps_restrict(
                cap_group, src_caps, nr, dst_caps, NULL, NULL);
}

int usys_transfer_caps_restrict(cap_t dest_group_cap, cap_t *src_caps,
                                int nr_caps, cap_t *dst_caps,
                                cap_right_t *masks, cap_right_t *rests)
{
        return chcore_syscall6(CHCORE_SYS_transfer_caps,
                               dest_group_cap,
                               (unsigned long)src_caps,
                               (unsigned long)nr_caps,
                               (unsigned long)dst_caps,
                               (unsigned long)masks,
                               (unsigned long)rests);
}

void usys_empty_syscall(void)
{
        chcore_syscall0(CHCORE_SYS_empty_syscall);
}

void usys_top(void)
{
        chcore_syscall0(CHCORE_SYS_top);
}

int usys_user_fault_register(cap_t notific_cap, vaddr_t msg_buffer)
{
        return chcore_syscall2(
                CHCORE_SYS_user_fault_register, notific_cap, msg_buffer);
}

int usys_user_fault_map(badge_t client_badge, vaddr_t fault_va,
                        vaddr_t remap_va, bool copy, vmr_prop_t perm)
{
        return chcore_syscall5(CHCORE_SYS_user_fault_map,
                               client_badge,
                               fault_va,
                               remap_va,
                               copy,
                               perm);
}

int usys_map_pmo_with_length(cap_t pmo_cap, vaddr_t addr, unsigned long perm,
                             size_t length)
{
        return chcore_syscall5(
                CHCORE_SYS_map_pmo, SELF_CAP, pmo_cap, addr, perm, length);
}

int usys_unmap_pmo_with_length(cap_t pmo_cap, unsigned long addr, size_t size)
{
        return chcore_syscall4(
                CHCORE_SYS_unmap_pmo, SELF_CAP, pmo_cap, addr, size);
}

cap_t usys_irq_register(int irq)
{
        return chcore_syscall1(CHCORE_SYS_irq_register, irq);
}

int usys_irq_wait(cap_t irq_cap, bool is_block)
{
        return chcore_syscall2(CHCORE_SYS_irq_wait, irq_cap, is_block);
}

int usys_irq_ack(cap_t irq_cap)
{
        return chcore_syscall1(CHCORE_SYS_irq_ack, irq_cap);

}

#ifdef CHCORE_ARCH_SPARC
void usys_disable_irq(void)
{
        chcore_syscall3(CHCORE_SYS_configure_irq, false, true, 0);
}

void usys_enable_irq(void)
{
        chcore_syscall3(CHCORE_SYS_configure_irq, true, true, 0);
}

void usys_enable_irqno(unsigned long irqno)
{
        chcore_syscall3(CHCORE_SYS_configure_irq, true, false, irqno);
}

void usys_disable_irqno(unsigned long irqno)
{
        chcore_syscall3(CHCORE_SYS_configure_irq, false, false, irqno);
}

void usys_cache_config(unsigned long option)
{
        chcore_syscall1(CHCORE_SYS_cache_config, option);
}
#endif /* CHCORE_ARCH_SPARC */

cap_t usys_create_notifc(void)
{
        return chcore_syscall0(CHCORE_SYS_create_notifc);
}

int usys_wait(cap_t notifc_cap, bool is_block, struct timespec *timeout)
{
        return chcore_syscall3(
                CHCORE_SYS_wait, notifc_cap, is_block, (unsigned long)timeout);
}

int usys_notify(cap_t notifc_cap)
{
        int ret;

        do {
                ret = chcore_syscall1(CHCORE_SYS_notify, notifc_cap);

                if (ret == -EAGAIN) {
                        // printf("%s retry\n", __func__);
                        usys_yield();
                }
        } while (ret == -EAGAIN);
        return ret;
}

/* Only used for recycle process */
int usys_register_recycle_thread(cap_t cap, unsigned long buffer)
{
        return chcore_syscall2(CHCORE_SYS_register_recycle, cap, buffer);
}

int usys_cap_group_recycle(cap_t cap)
{
        return chcore_syscall1(CHCORE_SYS_cap_group_recycle, cap);
}

int usys_ipc_close_connection(cap_t cap)
{
        return chcore_syscall1(CHCORE_SYS_ipc_close_connection, cap);
}

/* Get the size of free memory */
int usys_get_free_mem_size(struct free_mem_info *info)
{
        return chcore_syscall1(CHCORE_SYS_get_free_mem_size, (unsigned long)info);
}

void usys_get_mem_usage_msg(void)
{
        chcore_syscall0(CHCORE_SYS_get_mem_usage_msg);
}

int usys_cache_flush(unsigned long start, unsigned long size, int op_type)
{
        return chcore_syscall3(CHCORE_SYS_cache_flush, start, size, op_type);
}

unsigned long usys_get_current_tick(void)
{
        return chcore_syscall0(CHCORE_SYS_get_current_tick);
}

void usys_get_pci_device(int dev_class, unsigned long uaddr)
{
        chcore_syscall2(CHCORE_SYS_get_pci_device, dev_class, uaddr);
}

void usys_poweroff(void)
{
        chcore_syscall0(CHCORE_SYS_poweroff);
}

int usys_get_system_info(int op, void *ubuffer,
                         unsigned long size, long arg)
{
        return chcore_syscall4(CHCORE_SYS_get_system_info, op,
                               (unsigned long)ubuffer, 
                               size, arg);
}

int usys_ptrace(int req, unsigned long cap, unsigned long tid,
                void *addr, void *data)
{
        return chcore_syscall5(CHCORE_SYS_ptrace, req, cap, tid,
                               (unsigned long)addr, (unsigned long)data);
}

long usys_opentrustee(long z, long a, long b, long c, long d, long e, long f)
{
#ifdef CHCORE_OPENTRUSTEE
        return chcore_syscall7(CHCORE_SYS_opentrustee, z, a, b, c, d, e, f);
#else 
        return -ENOSYS;
#endif /* CHCORE_OPENTRUSTEE */
}
