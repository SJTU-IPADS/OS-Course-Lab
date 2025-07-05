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

#ifndef UAPI_OPENTRUSTEE_SYSCALL_NUM_H
#define UAPI_OPENTRUSTEE_SYSCALL_NUM_H

#define OPENTRUSTEE_NR_SYSCALL 32

#define OPENTRUSTEE_SYS_get_thread_id 5
#define OPENTRUSTEE_SYS_terminate_thread 6
#define OPENTRUSTEE_SYS_disable_local_irq 7
#define OPENTRUSTEE_SYS_enable_local_irq 8

#define OPENTRUSTEE_SYS_tee_msg_create_msg_hdl 9
#define OPENTRUSTEE_SYS_tee_msg_create_channel 10
#define OPENTRUSTEE_SYS_tee_msg_receive 11
#define OPENTRUSTEE_SYS_tee_msg_call 12
#define OPENTRUSTEE_SYS_tee_msg_reply 13
#define OPENTRUSTEE_SYS_tee_msg_notify 14
#define OPENTRUSTEE_SYS_tee_msg_stop_channel 15

#define OPENTRUSTEE_SYS_tee_wait_switch_req 16
#define OPENTRUSTEE_SYS_tee_switch_req 17
#define OPENTRUSTEE_SYS_tee_create_ns_pmo 18
#define OPENTRUSTEE_SYS_tee_pull_kernel_var 19

#define OPENTRUSTEE_SYS_tee_push_rdr_update_addr 20
#define OPENTRUSTEE_SYS_debug_rdr_logitem 21

#endif /* UAPI_OPENTRUSTEE_SYSCALL_NUM_H */
