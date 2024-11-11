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
#include <pthread.h>
#include <chcore/ipc.h>

#include <chcore/type.h>

/* Unify the length of uart buffer and foreground process' input buffer. */
#define INPUT_BUFLEN (4096 - sizeof(int) * 2)
#define NR_ARGS_MAX  64
#define JOBS_MAX     64

#define CTRL(ch) (ch & 0x1f)

extern void *foreground_buffer_addr;
extern void *foreground_pmo_addr;

struct job {
        pid_t pid;
        cap_t notific_cap;
        cap_t pmo_cap;
        char job_name[64];
};

extern struct job *bg_jobs[JOBS_MAX];
extern struct job fg_job;
extern int job_count;
extern pthread_mutex_t job_mutex;

int check_job_state(int pid);
void clean_jobs(void);
int add_job(char *job_name, int pid, cap_t pmo_cap, cap_t notific_cap);
void del_job(long index);
