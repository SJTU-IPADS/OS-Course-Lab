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

#ifndef UAPI_GET_SYSTEM_INFO_H
#define UAPI_GET_SYSTEM_INFO_H

/* thread information struct */
struct thread_info
{
        /* Thread state */
        unsigned int state;
        /* Thread type */
        unsigned int type;
        /* Current assigned CPU */
        unsigned int cpuid;
        /* SMP affinity */
        int affinity;
        /* Thread priority */
        int prio;
};

/* process information struct */
struct process_info
{
        /* process name */
        char cap_group_name[64];
        /* thread number */
        int thread_num;
        /* physical memory ss */
        unsigned long rss;
        /* virtual memory ss */
        unsigned long vss;
};

enum gsi_op {
        GSI_IRQ,
        GSI_PROCESS,
        GSI_THREAD
};

int sys_get_system_info(enum gsi_op op, void *ubuffer,
                        unsigned long size, long arg);

#endif /* UAPI_GET_SYSTEM_INFO_H */
