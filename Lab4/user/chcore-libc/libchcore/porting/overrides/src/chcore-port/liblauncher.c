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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chcore/bug.h>
#include <chcore/defs.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore/proc.h>
#include <chcore/ipc.h>
#include <chcore/syscall.h>
#include <chcore/launcher.h>
#include <chcore/memory.h>

static pid_t chcore_new_process_internal(int argc, char *argv[], int type)
{
        int ret;
        int i;
        struct proc_request *pr;
        ipc_msg_t *ipc_msg;
        int text_i;

        ipc_msg = ipc_create_msg(
                procmgr_ipc_struct, sizeof(struct proc_request));
        pr = (struct proc_request *)ipc_get_msg_data(ipc_msg);

        pr->req = PROC_REQ_NEWPROC;
        pr->newproc.proc_type = type;
        pr->newproc.argc = argc;

        /* Dump argv[] to the text area of the pr. */
        for (i = 0, text_i = 0; i < argc; ++i) {
                /* Plus 1 for the trailing \0. */
                int len = strlen(argv[i]) + 1;
                assert(text_i + len <= PROC_REQ_TEXT_SIZE);

                memcpy(&pr->newproc.argv_text[text_i], argv[i], len);

                pr->newproc.argv_off[i] = text_i;
                text_i += len;
        }

        ret = ipc_call(procmgr_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

pid_t chcore_new_process(int argc, char *argv[])
{
        return chcore_new_process_internal(argc, argv, COMMON_APP);
}

int get_new_process_mt_cap(int pid)
{
        ipc_msg_t *ipc_msg;
        struct proc_request *pr;
        cap_t mt_cap;

        ipc_msg = ipc_create_msg(
                procmgr_ipc_struct, sizeof(struct proc_request));

        pr = (struct proc_request *)ipc_get_msg_data(ipc_msg);
        pr->req = PROC_REQ_GET_MT_CAP;
        pr->get_mt_cap.pid = pid;

        ipc_call(procmgr_ipc_struct, ipc_msg);
        mt_cap = ipc_get_msg_cap(ipc_msg, 0);
        ipc_destroy_msg(ipc_msg);

        return mt_cap;
}

int create_process(int argc, char *argv[], struct new_process_caps *caps)
{
        pid_t pid;
        pid = chcore_new_process(argc, argv);
        if (caps != NULL && pid >= 0)
                caps->mt_cap = get_new_process_mt_cap(pid);
        return pid;
}

int create_traced_process(int argc, char *argv[], struct new_process_caps *caps)
{
        pid_t pid;
        pid = chcore_new_process_internal(argc, argv, TRACED_APP);
        if (caps != NULL && pid >= 0)
                caps->mt_cap = get_new_process_mt_cap(pid);
        return pid;
}
