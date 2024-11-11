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

#include <common/errno.h>
#ifdef CHCORE_OPENTRUSTEE
#include <object/memory.h>
#include <object/thread.h>
#include <arch/trustzone/smc.h>
#include <arch/trustzone/tlogger.h>
#include <ipc/channel.h>
#include <uapi/opentrustee/syscall_num.h>
#include <syscall/opentrustee.h>
#include <object/recycle.h>

/* Placeholder for system calls that are not implemented */
int ot_sys_null_placeholder(void)
{
        kwarn("Invoke non-implemented opentrustee syscall\n");
        return -EBADSYSCALL;
}

const void *ot_syscall_table[OPENTRUSTEE_NR_SYSCALL] = {
        [0 ... OPENTRUSTEE_NR_SYSCALL - 1] = ot_sys_null_placeholder,

        [OPENTRUSTEE_SYS_get_thread_id] = ot_sys_get_thread_id,
        [OPENTRUSTEE_SYS_terminate_thread] = ot_sys_terminate_thread,
        [OPENTRUSTEE_SYS_disable_local_irq] = ot_sys_disable_local_irq,
        [OPENTRUSTEE_SYS_enable_local_irq] = ot_sys_enable_local_irq,

        [OPENTRUSTEE_SYS_tee_msg_create_msg_hdl] = ot_sys_tee_msg_create_msg_hdl,
        [OPENTRUSTEE_SYS_tee_msg_create_channel] = ot_sys_tee_msg_create_channel,
        [OPENTRUSTEE_SYS_tee_msg_receive] = ot_sys_tee_msg_receive,
        [OPENTRUSTEE_SYS_tee_msg_call] = ot_sys_tee_msg_call,
        [OPENTRUSTEE_SYS_tee_msg_reply] = ot_sys_tee_msg_reply,
        [OPENTRUSTEE_SYS_tee_msg_notify] = ot_sys_tee_msg_notify,
        [OPENTRUSTEE_SYS_tee_msg_stop_channel] = ot_sys_tee_msg_stop_channel,

        [OPENTRUSTEE_SYS_tee_wait_switch_req] = ot_sys_tee_wait_switch_req,
        [OPENTRUSTEE_SYS_tee_switch_req] = ot_sys_tee_switch_req,
        [OPENTRUSTEE_SYS_tee_pull_kernel_var] = ot_sys_tee_pull_kernel_var,
        
        [OPENTRUSTEE_SYS_tee_push_rdr_update_addr] = ot_sys_tee_push_rdr_update_addr,
        [OPENTRUSTEE_SYS_debug_rdr_logitem] = ot_sys_debug_rdr_logitem,
};

long sys_opentrustee(long z, long a, long b, long c,
                    long d, long e, long f)
{
        long (*func)(long, long, long, long, long, long);
        func = ot_syscall_table[z];
        return func(a, b, c, d, e, f);
}
#else /* CHCORE_OPENTRUSTEE */

long sys_opentrustee(long z, long a, long b, long c,
                    long d, long e, long f)
{
        return -ENOSYS;
}
#endif /* CHCORE_OPENTRUSTEE */
