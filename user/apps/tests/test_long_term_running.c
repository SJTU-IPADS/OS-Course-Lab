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

#include "chcore/launcher.h"
#include "stdbool.h"
#include <stdio.h>
#include <stdlib.h>
#include <chcore/proc.h>
#include <chcore/container/list.h>
#include <chcore/syscall.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

#define TEST_WORKERS        (4)
#define TEST_NUM            (100)
#define TEST_CANDIDATE_JOBS (11)
#define TEST_OUT_FILE       "/ltr.txt"

#define fatal(fmt, ...) printf("[FATAL] [Controller] " fmt, ##__VA_ARGS__)
#define info(fmt, ...)  printf("[INFO] [Controller] " fmt, ##__VA_ARGS__)
#define debug(fmt, ...) printf("[DEBUG] [Controller] " fmt, ##__VA_ARGS__)

const char* candidate_jobs[TEST_CANDIDATE_JOBS] = {
        "/test_net.bin",
        "/test_sched.bin",
        "/test_fpu.bin",
        "/test_epoll.bin",
        "/test_poll_new.bin",
        "/test_notifc.bin",
        "/test_pthread.bin",
        "/fs_test_full.bin",
        "/test_eventfd.bin",
        "/test_sleep.bin",
        "/test_timerfd.bin"};

struct test_job {
        struct list_head node;
        const char* cmd;
        int job_nr;
        int stop_job;
        int ret;
        struct timespec start;
};

struct test_job* new_job(const char* cmd, int job_nr)
{
        struct test_job* p = malloc(sizeof(*p));
        if (!p) {
                return NULL;
        }

        p->cmd = cmd;
        p->job_nr = job_nr;
        p->stop_job = false;
        return p;
}

struct test_job* new_stop_job(void)
{
        struct test_job* p = malloc(sizeof(*p));
        p->stop_job = true;
        return p;
}

struct job_queue {
        pthread_mutex_t mu;
        struct list_head head;
};

void job_queue_init(struct job_queue* this)
{
        pthread_mutex_init(&this->mu, NULL);

        init_list_head(&this->head);
}

void job_enqueue(struct job_queue* this, struct test_job* job)
{
        pthread_mutex_lock(&this->mu);

        list_append(&job->node, &this->head);

        pthread_mutex_unlock(&this->mu);
}

struct test_job* job_dequeue(struct job_queue* this)
{
        struct test_job* ret = NULL;
        struct list_head* node;
        pthread_mutex_lock(&this->mu);

        if (list_empty(&this->head)) {
                goto out;
        }

        node = this->head.next;
        ret = list_entry(node, struct test_job, node);
        list_del(node);

out:
        pthread_mutex_unlock(&this->mu);
        return ret;
}

bool job_queue_empty(struct job_queue* this)
{
        bool ret;
        pthread_mutex_lock(&this->mu);
        ret = list_empty(&this->head);
        pthread_mutex_unlock(&this->mu);
        return ret;
}

struct job_controller {
        pthread_mutex_t mu;
        pthread_cond_t generate_job_cond;
        bool job_running_status[TEST_CANDIDATE_JOBS];

        int available_jobs;
        int available_workers;
};

void job_controller_init(struct job_controller* this)
{
        pthread_mutex_init(&this->mu, NULL);
        pthread_cond_init(&this->generate_job_cond, NULL);

        memset(&this->job_running_status,
               false,
               sizeof(this->job_running_status));
        this->available_jobs = TEST_CANDIDATE_JOBS;
        this->available_workers = TEST_WORKERS;
}

struct test_job* _job_controller_generate(struct job_controller* this)
{
        struct test_job* job = NULL;
        int candidate;

        if (!(this->available_workers > 0 && this->available_jobs > 0)) {
                debug("no available workers/jobs!\n");
                goto out;
        }

        do {
                candidate = rand() % TEST_CANDIDATE_JOBS;
                debug("candidate: %d, status: %d\n",
                      candidate,
                      this->job_running_status[candidate]);
        } while (this->job_running_status[candidate]);

        job = new_job(candidate_jobs[candidate], candidate);
        this->available_jobs--;
        this->available_workers--;
        this->job_running_status[candidate] = true;
out:
        return job;
}

struct test_job* job_controller_generate_job(struct job_controller* this)
{
        struct test_job* ret = NULL;

        pthread_mutex_lock(&this->mu);
        debug("hold controller lock!\n");
        while (!(ret = _job_controller_generate(this))) {
                debug("generate failed, enter sleep!\n");
                pthread_cond_wait(&this->generate_job_cond, &this->mu);
        }

        pthread_mutex_unlock(&this->mu);

        return ret;
}

void job_controller_confirm_job(struct job_controller* this, int worker_id,
                                struct test_job* job)
{
        pthread_mutex_lock(&this->mu);

        this->job_running_status[job->job_nr] = false;
        this->available_jobs++;
        this->available_workers++;
        free(job);

        pthread_mutex_unlock(&this->mu);
        pthread_cond_signal(&this->generate_job_cond);
}

struct worker {
        int id;
        struct job_controller* controller;
        FILE* output;
        struct job_queue* job_queue;
};

struct worker* new_worker(int id, struct job_controller* controller,
                          FILE* output, struct job_queue* queue)
{
        struct worker* p = malloc(sizeof(*p));
        if (!p) {
                return NULL;
        }

        p->id = id;
        p->controller = controller;
        p->output = output;
        p->job_queue = queue;

        return p;
}

struct timespec diff_timespec(const struct timespec* time1,
                              const struct timespec* time0)
{
        struct timespec diff = {.tv_sec = time1->tv_sec - time0->tv_sec,
                                .tv_nsec = time1->tv_nsec - time0->tv_nsec};
        if (diff.tv_nsec < 0) {
                diff.tv_nsec += 1000000000;
                diff.tv_sec--;
        }
        return diff;
}

void _worker_collect_log_stats(struct worker* this, struct test_job* job)
{
        struct timespec cur_time, diff;
        struct free_mem_info free_mem_info;
        long mem;
        int ret;
        clock_gettime(CLOCK_MONOTONIC, &cur_time);
        ret = usys_get_free_mem_size(&free_mem_info);

        if (ret < 0) {
                mem = ret; // to be reported in log
        } else {
                mem = free_mem_info.free_mem_size;
        }

        diff = diff_timespec(&cur_time, &job->start);
        fprintf(this->output,
                "[INFO] [Worker %d] job_cmd: %s job_ret: %d free_mem_size: %lx bytes, start: %" PRId64 "s:%ldns take_time: %" PRId64 "s:%ldns\n",
                this->id,
                job->cmd,
                job->ret,
                mem,
                job->start.tv_sec,
                job->start.tv_nsec,
                diff.tv_sec,
                diff.tv_nsec);
        printf("[INFO] [Worker %d] job_cmd: %s job_ret: %d free_mem_size: %lx bytes, start: %" PRId64 "s:%ldns take_time: %" PRId64 "s:%ldns\n",
               this->id,
               job->cmd,
               job->ret,
               mem,
               job->start.tv_sec,
               job->start.tv_nsec,
               diff.tv_sec,
               diff.tv_nsec);
}

void* worker_run(void* param)
{
        struct worker* worker = param;
        struct test_job* job;
        int status, ret;

        while (true) {
                job = job_dequeue(worker->job_queue);
                if (!job) {
                        continue;
                }

                if (job->stop_job) {
                        free(job);
                        break;
                }

                clock_gettime(CLOCK_MONOTONIC, &job->start);

                ret = chcore_new_process(1, (char**)&job->cmd);
                if (ret < 0) {
                        job->ret = ret;
                        goto confirm;
                }

                chcore_waitpid(ret, &status, 0, 0);
                job->ret = status;

        confirm:
                _worker_collect_log_stats(worker, job);
                job_controller_confirm_job(worker->controller, worker->id, job);
        }

        return NULL;
}

int main()
{
        int i;
        FILE* output = fopen(TEST_OUT_FILE, "w");
        struct worker* workers[TEST_WORKERS];
        pthread_t worker_tids[TEST_WORKERS];
        struct job_controller* controller = malloc(sizeof(*controller));
        struct job_queue* job_queue = malloc(sizeof(*job_queue));
        struct test_job* job;
        srand(time(NULL));

        job_controller_init(controller);
        job_queue_init(job_queue);

        for (i = 0; i < TEST_WORKERS; i++) {
                workers[i] = new_worker(i, controller, output, job_queue);
        }

        info("finished initialing workers...\n");

        for (i = 0; i < TEST_WORKERS; i++) {
                pthread_create(&worker_tids[i], NULL, worker_run, workers[i]);
        }

        info("finished creating worker threads...\n");

        for (i = 0; i < TEST_NUM; i++) {
                job = job_controller_generate_job(controller);
                info("job %d/%d generated...\n", i + 1, TEST_NUM);
                job_enqueue(job_queue, job);
        }

        for (i = 0; i < TEST_WORKERS; i++) {
                job = new_stop_job();
                job_enqueue(job_queue, job);
        }

        for (i = 0; i < TEST_WORKERS; i++) {
                pthread_join(worker_tids[i], NULL);
        }

        info("successfully performed long term running tests, congrat!\n");

        return 0;
}