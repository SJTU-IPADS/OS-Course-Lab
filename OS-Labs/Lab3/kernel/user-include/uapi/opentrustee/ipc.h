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

#ifndef UAPI_OPENTRUSTEE_IPC_H
#define UAPI_OPENTRUSTEE_IPC_H

#define PID_OFFSET 16U
#define PID_MASK   ((0x1 << PID_OFFSET) - 1)
#define pid_to_taskid(tid, pid) \
        (((u32)(tid) << PID_OFFSET) | ((u32)(pid)&PID_MASK))
#define taskid_to_pid(task_id) ((task_id)&PID_MASK)
#define taskid_to_tid(task_id) (((task_id) >> PID_OFFSET) & TID_MASK)

#endif /* UAPI_OPENTRUSTEE_IPC_H */
