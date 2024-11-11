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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include <chcore/bug.h>
#include <sys/mman.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore/string.h>

#include "job_control.h"

/*
 * When a foreground process want to receive user's input at the first time, it
 * will send the information below to shell, and shell will map the buffer pmo
 * to it's memory space. When the foreground process exits or become a
 * background process, shell will unmap the buffer pmo. When the user input some
 * character, shell will receive the character, fill into the foreground buffer
 * and awake the foreground process.
 */
void *foreground_buffer_addr = NULL;
void *foreground_pmo_addr = NULL;
struct job fg_job = {.pid = -1, .notific_cap = -1, .pmo_cap = -1};

struct job *bg_jobs[JOBS_MAX] = {NULL};
int job_count = 0;

/* 
 * Notice: All operaions refer to bg_jobs need require the job_mutex lock first.
 */
pthread_mutex_t job_mutex;

int check_job_state(int pid)
{
        int ret = 0;
        struct proc_request *proc_req;

        ipc_msg_t *msg =
                ipc_create_msg(procmgr_ipc_struct, sizeof(struct proc_request));
        proc_req = (struct proc_request *)ipc_get_msg_data(msg);
        proc_req->check_state.pid = pid;
        proc_req->req = PROC_REQ_CHECK_STATE;
        ret = (int)ipc_call(procmgr_ipc_struct, msg);
        ipc_destroy_msg(msg);

        return ret;
}

static int find_free_pos(void)
{
        int pos = 0;

        for (pos = 0; pos < JOBS_MAX; pos++) {
                if (bg_jobs[pos] == NULL) {
                        return pos;
                }
        }

        return -1;
}

/*
 * If there exist a process with the same pid in the bg_jobs array, it returns
 * the job index. Otherwise, it returns -1.
 */
int is_job_exist(pid_t pid)
{
        int i = 0, j = 0;
        int total_jobs = job_count;
        for (i = 0; i < total_jobs && j < JOBS_MAX;) {
                if (bg_jobs[j] != NULL) {
                        if (pid == bg_jobs[j]->pid) {
                                /*
                                 * Old process has exited but shell has not
                                 * cleaned the process infomation.
                                 */
                                return j;
                        }
                        i++;
                }
                j++;
        }
        return -1;
}

/* Return the pos that the job locate in. */
int add_job(char *job_name, int pid, cap_t pmo_cap, cap_t notific_cap)
{
        int pos;
        struct job *new_job;

        pos = is_job_exist(pid);
        if (pos == -1) {
                new_job = (struct job *)malloc(sizeof(struct job));
                if (new_job == NULL) {
                        return -1;
                }
                pos = find_free_pos();
                if (pos < 0) {
                        free(new_job);
                        return -1;
                }

                bg_jobs[pos] = new_job;
                job_count++;
        } else {
                new_job = bg_jobs[pos];
        }
        strlcpy(new_job->job_name, job_name, sizeof(new_job->job_name));
        new_job->pid = pid;
        new_job->pmo_cap = pmo_cap;
        new_job->notific_cap = notific_cap;

        return pos;
}

void del_job(long index)
{
        BUG_ON(index < 0);
        free(bg_jobs[index]);
        job_count--;
        bg_jobs[index] = NULL;
}

void clean_jobs(void)
{
        int i = 0, j = 0;
        int total_jobs = job_count;
        for (i = 0; i < total_jobs && j < JOBS_MAX;) {
                if (bg_jobs[j] != NULL) {
                        if (check_job_state(bg_jobs[j]->pid) == 1) {
                                del_job(j);
                                bg_jobs[j] = NULL;
                        }
                        i++;
                }
                j++;
        }
}
