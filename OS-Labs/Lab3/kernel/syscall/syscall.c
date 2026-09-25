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

#include <common/types.h>
#include <io/uart.h>
#include <mm/uaccess.h>
#include <mm/kmalloc.h>
#include <mm/cache.h>
#include <mm/mm.h>
#include <common/kprint.h>
#include <common/debug.h>
#include <common/lock.h>
#include <object/memory.h>
#include <object/thread.h>
#include <object/cap_group.h>
#include <object/recycle.h>
#include <object/object.h>
#include <object/irq.h>
#include <object/user_fault.h>
#include <object/ptrace.h>
#include <sched/sched.h>
#include <ipc/connection.h>
#include <ipc/futex.h>
#include <irq/timer.h>
#include <irq/irq.h>
#include <common/poweroff.h>
#include <uapi/get_system_info.h>
#include <syscall/opentrustee.h>

#ifdef CHCORE_ARCH_X86_64
#include <arch/pci.h>
#endif /* CHCORE_ARCH_X86_64 */

#include <uapi/syscall_num.h>

#if ENABLE_HOOKING_SYSCALL == ON
void hook_syscall(long n)
{
        if ((n != CHCORE_SYS_putstr) && (n != CHCORE_SYS_getc) && (n != CHCORE_SYS_yield)
            && (n != CHCORE_SYS_handle_brk))
                kinfo("[SYSCALL TRACING] hook_syscall num: %ld\n", n);
}
#endif

/* Placeholder for system calls that are not implemented */
int sys_null_placeholder(void)
{
        kwarn("Invoke non-implemented syscall\n");
        return -EBADSYSCALL;
}

#if ENABLE_PRINT_LOCK == ON
DEFINE_SPINLOCK(global_print_lock);
#endif

void sys_putstr(char *str, size_t len)
{
        if (check_user_addr_range((vaddr_t)str, len) != 0)
                return;

#define PRINT_BUFSZ 64
        char buf[PRINT_BUFSZ];
        size_t copy_len;
        size_t i;
        int r;

        do {
                copy_len = (len > PRINT_BUFSZ) ? PRINT_BUFSZ : len;
                r = copy_from_user(buf, str, copy_len);
                if (r)
                        return;

#if ENABLE_PRINT_LOCK == ON
                lock(&global_print_lock);
#endif
                for (i = 0; i < copy_len; ++i) {
                        uart_send((unsigned int)buf[i]);
                }

#if ENABLE_PRINT_LOCK == ON
                unlock(&global_print_lock);
#endif
                len -= copy_len;
                str += copy_len;
        } while (len != 0);
}

char sys_getc(void)
{
        return nb_uart_recv();
}

/* Helper system calls for user-level drivers to use. */
int sys_cache_flush(unsigned long start, long len, int op_type)
{
        arch_flush_cache(start, len, op_type);
        return 0;
}

unsigned long sys_get_current_tick(void)
{
        return plat_get_current_tick();
}

/* An empty syscall for measuring the syscall overhead. */
void sys_empty_syscall(void)
{
}

void sys_get_pci_device(int class, u64 pci_dev_uaddr)
{
#ifdef CHCORE_ARCH_X86_64
        arch_get_pci_device(class, pci_dev_uaddr);
#endif
}

void sys_poweroff(void)
{
        plat_poweroff();
}

const void *syscall_table[NR_SYSCALL] = {
        [0 ... NR_SYSCALL - 1] = sys_null_placeholder,

        /* Character IO */
        [CHCORE_SYS_putstr] = sys_putstr,
        [CHCORE_SYS_getc] = sys_getc,

        /* PMO */
        [CHCORE_SYS_create_pmo] = sys_create_pmo,
        [CHCORE_SYS_map_pmo] = sys_map_pmo,
        [CHCORE_SYS_unmap_pmo] = sys_unmap_pmo,
        [CHCORE_SYS_write_pmo] = sys_write_pmo,
        [CHCORE_SYS_read_pmo] = sys_read_pmo,
        /* - address translation */
        [CHCORE_SYS_get_phys_addr] = sys_get_phys_addr,

        /* Capability */
        [CHCORE_SYS_revoke_cap] = sys_revoke_cap,
        [CHCORE_SYS_transfer_caps] = sys_transfer_caps,

        /* Multitask */
        /* - create & exit */
        [CHCORE_SYS_create_cap_group] = sys_create_cap_group,
        [CHCORE_SYS_exit_group] = sys_exit_group,
        [CHCORE_SYS_kill_group] = sys_kill_group,
        [CHCORE_SYS_create_thread] = sys_create_thread,
        [CHCORE_SYS_thread_exit] = sys_thread_exit,
        /* - recycle */
        [CHCORE_SYS_register_recycle] = sys_register_recycle,
        [CHCORE_SYS_cap_group_recycle] = sys_cap_group_recycle,
        [CHCORE_SYS_ipc_close_connection] = sys_ipc_close_connection,
        /* - schedule */
        [CHCORE_SYS_yield] = sys_yield,
        [CHCORE_SYS_set_affinity] = sys_set_affinity,
        [CHCORE_SYS_get_affinity] = sys_get_affinity,
        [CHCORE_SYS_set_prio] = sys_set_prio,
        [CHCORE_SYS_get_prio] = sys_get_prio,
        [CHCORE_SYS_suspend] = sys_suspend,
        [CHCORE_SYS_resume] = sys_resume,
        /* ptrace */
	[CHCORE_SYS_ptrace] = sys_ptrace,

        /* IPC */
        /* - procedure call */
        [CHCORE_SYS_register_server] = sys_register_server,
        [CHCORE_SYS_register_client] = sys_register_client,
        [CHCORE_SYS_ipc_register_cb_return] = sys_ipc_register_cb_return,
        [CHCORE_SYS_ipc_call] = sys_ipc_call,
        [CHCORE_SYS_ipc_return] = sys_ipc_return,
        [CHCORE_SYS_ipc_exit_routine_return] = sys_ipc_exit_routine_return,
        [CHCORE_SYS_ipc_get_cap] = sys_ipc_get_cap,
        [CHCORE_SYS_ipc_set_cap] = sys_ipc_set_cap,
        /* - notification */
        [CHCORE_SYS_create_notifc] = sys_create_notifc,
        [CHCORE_SYS_wait] = sys_wait,
        [CHCORE_SYS_notify] = sys_notify,

        /* Exception */
        /* - irq */
        [CHCORE_SYS_irq_register] = sys_irq_register,
        [CHCORE_SYS_irq_wait] = sys_irq_wait,
        [CHCORE_SYS_irq_ack] = sys_irq_ack,
#ifdef CHCORE_ARCH_SPARC
        [CHCORE_SYS_configure_irq] = sys_configure_irq,
        [CHCORE_SYS_cache_config] = sys_cache_config,
#endif
        /* - page fault */
        [CHCORE_SYS_user_fault_register] = sys_user_fault_register,
        [CHCORE_SYS_user_fault_map] = sys_user_fault_map,

        /* POSIX */
        /* - time */
        [CHCORE_SYS_clock_gettime] = sys_clock_gettime,
        [CHCORE_SYS_clock_nanosleep] = sys_clock_nanosleep,
        /* - memory */
        [CHCORE_SYS_handle_brk] = sys_handle_brk,
        [CHCORE_SYS_handle_mprotect] = sys_handle_mprotect,

        /* Hardware Access */
        [CHCORE_SYS_cache_flush] = sys_cache_flush,
        [CHCORE_SYS_get_current_tick] = sys_get_current_tick,
        [CHCORE_SYS_get_pci_device] = sys_get_pci_device,
        [CHCORE_SYS_poweroff] = sys_poweroff,

        /* Utils */
        [CHCORE_SYS_empty_syscall] = sys_empty_syscall,
        [CHCORE_SYS_top] = sys_top,
        [CHCORE_SYS_get_free_mem_size] = sys_get_free_mem_size,
        [CHCORE_SYS_get_mem_usage_msg] = get_mem_usage_msg,
        [CHCORE_SYS_get_system_info] = sys_get_system_info,

        /* - futex */
        [CHCORE_SYS_futex] = sys_futex,      
        [CHCORE_SYS_set_tid_address] = sys_set_tid_address,

        [CHCORE_SYS_opentrustee] = sys_opentrustee,

};
