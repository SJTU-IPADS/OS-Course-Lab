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

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <chcore/launcher.h>
#include <chcore/defs.h>
#include <chcore/ipc.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore/syscall.h>
#include <chcore/opentrustee/syscall.h>
#include <chcore/pthread.h>

#include <spawn_ext.h>

static int __info_proc_by_pid(pid_t pid, struct proc_info *info)
{
        ipc_msg_t *ipc_msg;
        struct proc_request *req;
        int ret;

        ipc_msg =
                ipc_create_msg(procmgr_ipc_struct, sizeof(struct proc_request));
        req = (struct proc_request *)ipc_get_msg_data(ipc_msg);
        req->req = PROC_REQ_INFO_PROC_BY_PID;

        req->info_proc_by_pid.pid = pid;
        ret = ipc_call(procmgr_ipc_struct, ipc_msg);

        if (ret != 0) {
                goto out;
        }

        memcpy(info, &req->info_proc_by_pid.info, sizeof(*info));

out:
        ipc_destroy_msg(ipc_msg);
        return ret;
}

int getuuid(pid_t pid, spawn_uuid_t *uuid)
{
        struct proc_info info;
        int ret;

        ret = __info_proc_by_pid(pid, &info);
        if (ret == 0) {
                memcpy(uuid, &info.uuid, sizeof(*uuid));
        }

        return ret;
}

#define DEFAULT_STACK_SIZE 0x800000UL
#define DEFAULT_HEAP_SIZE  0x8000000UL

int spawnattr_init(posix_spawnattr_t *attr)
{
        *attr = (posix_spawnattr_t){0};
        attr->stack_size = DEFAULT_STACK_SIZE;
        attr->heap_size = DEFAULT_HEAP_SIZE;
        return 0;
}

void spawnattr_setuuid(posix_spawnattr_t *attr, const spawn_uuid_t *uuid)
{
        attr->uuid = *uuid;
}

int spawnattr_setheap(posix_spawnattr_t *attr, size_t size)
{
        attr->heap_size = size;
        return 0;
}

int spawnattr_setstack(posix_spawnattr_t *attr, size_t size)
{
        attr->stack_size = size;
        return 0;
}

int32_t thread_terminate(pthread_t thread)
{
        int tid, ret;

        tid = chcore_pthread_get_tid(thread);
        ret = usys_terminate_thread(tid);

        chcore_pthread_wake_joiner(thread);

        return ret;
}

int posix_spawn_ex(pid_t *pid, const char *path,
                   const posix_spawn_file_actions_t *file_action,
                   const posix_spawnattr_t *attrp, char **argv, char **envp,
                   int *tid)
{
        int ret;
        int argc = 0;

        if (argv != NULL) {
                while (argv[argc] != NULL) {
                        argc++;
                }
        }

        ret = chcore_new_process_spawn(argc, argv, envp, attrp, tid, path);
        if (ret >= 0) {
                if (pid != NULL) {
                        *pid = ret;
                }
                ret = 0;
        }
        return ret;
}

size_t getstacksize(void)
{
        struct proc_info info;
        int ret;

        ret = __info_proc_by_pid(getpid(), &info);
        if (ret == 0) {
                return info.stack_size;
        }

        return 0;
}
