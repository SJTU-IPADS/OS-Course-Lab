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

#include <common/lock.h>
#include <common/kprint.h>
#include <arch/machine/smp.h>
#include <common/macro.h>
#include <mm/kmalloc.h>
#include <object/thread.h>
#include <sched/context.h>
#include <sched/sched.h>
#include <machine.h>

#define TEST_NUM        512
#define THREAD_NUM      512

volatile int sched_start_flag = 0;
volatile int sched_finish_flag = 0;

struct thread *rr_sched_choose_thread(void);
int rr_sched_enqueue(struct thread *thread);
void rr_sched(void);

/* Metadata for ready queue */
struct queue_meta {
        struct list_head queue_head;
        u32 queue_len;
        struct lock queue_lock;
        char pad[pad_to_cache_line(sizeof(u32) + sizeof(struct list_head)
                                   + sizeof(struct lock))];
};

extern struct queue_meta rr_ready_queue_meta[PLAT_CPU_NUM];

void tst_rr(void);

int pb_sched_enqueue(struct thread *thread);
int pb_sched_dequeue(struct thread *thread);
struct thread *pbrr_sched_choose_thread(void);
int pbrr_sched(void);
void tst_pbrr(void);

static int is_valid_prev_thread(struct thread *thread)
{
        if ((thread == NULL) || (thread == THREAD_ITSELF))
                return 0;
        return 1;
}

/* Test scheduler */
void tst_sched(void)
{
        if (cur_sched_ops == &rr)
                tst_rr();
        else if (cur_sched_ops == &pbrr)
                tst_pbrr();
        /* No other policy supported */
        if (smp_get_cpu_id() == 0) {
                kinfo("[TEST] sched succ!\n");
        }
}

void tst_rr(void)
{
        int cpuid = 0;
        struct thread *thread = 0;

        cpuid = smp_get_cpu_id();
        /* check each queue shoule be empty */
        BUG_ON(!list_empty(&(rr_ready_queue_meta[cpuid].queue_head)));
        /* should return idle thread */
        thread = rr_sched_choose_thread();
        BUG_ON(thread->thread_ctx->type != TYPE_IDLE);

        /* ============ Start Barrier ============ */
        lock(&big_kernel_lock);
        sched_start_flag++;
        unlock(&big_kernel_lock);
        while (sched_start_flag != PLAT_CPU_NUM)
                ;
        /* ============ Start Barrier ============ */

        int i = 0, j = 0, k = 0, prio = 0;
        struct thread_ctx *thread_ctx = NULL;

        /* Init threads */
        for (i = 0; i < THREAD_NUM; i++) {
                for (j = 0; j < PLAT_CPU_NUM; j++) {
                        prio = ((i + j) % (MAX_PRIO - 1)) + 1;

                        thread = obj_alloc(TYPE_THREAD, sizeof(*thread));
                        obj_ref(thread);
                        BUG_ON(!(thread->thread_ctx =
                                         create_thread_ctx(TYPE_TESTS)));
                        init_thread_ctx(thread, 0, 0, prio, TYPE_TESTS, j);
                        for (k = 0; k < REG_NUM; k++)
                                thread->thread_ctx->ec.reg[k] = prio;
                        BUG_ON(rr_sched_enqueue(thread));
                }
        }

        for (j = 0; j < TEST_NUM; j++) {
                /* Each core try to get those threads */
                for (i = 0; i < THREAD_NUM * PLAT_CPU_NUM; i++) {
                        /* get thread and dequeue from ready queue */
                        do {
                                /* do it again if choose idle thread */
                                rr_sched();
                                BUG_ON(!current_thread);
                                BUG_ON(!current_thread->thread_ctx);
                                current_thread->thread_ctx->sc->budget = 0;

                                if (is_valid_prev_thread(
                                            current_thread->prev_thread))
                                        current_thread->prev_thread->thread_ctx
                                                ->kernel_stack_state = KS_FREE;

                        } while (current_thread->thread_ctx->type == TYPE_IDLE);
                        BUG_ON(!current_thread->thread_ctx->sc);
                        /* Current thread set affinitiy */
                        current_thread->thread_ctx->affinity =
                                (i + cpuid) % PLAT_CPU_NUM;
                        thread_ctx = (struct thread_ctx *)switch_context();

                        for (k = 0; k < REG_NUM; k++)
                                BUG_ON(thread_ctx->ec.reg[k]
                                       != thread_ctx->sc->prio);
                }
        }

        for (i = 0; i < THREAD_NUM * PLAT_CPU_NUM; i++) {
                /* get thread and dequeue from ready queue */
                do {
                        /* do it again if choose idle thread */
                        rr_sched();
                        BUG_ON(!current_thread);
                        BUG_ON(!current_thread->thread_ctx);
                        current_thread->thread_ctx->sc->budget = 0;

                        if (is_valid_prev_thread(current_thread->prev_thread))
                                current_thread->prev_thread->thread_ctx
                                        ->kernel_stack_state = KS_FREE;

                } while (current_thread->thread_ctx->type == TYPE_IDLE);
                BUG_ON(!current_thread->thread_ctx->sc);
                destroy_thread_ctx(current_thread);
                kfree(container_of(current_thread, struct object, opaque));
                current_thread = NULL;
        }

        /* ============ Finish Barrier ============ */
        lock(&big_kernel_lock);
        sched_finish_flag++;
        unlock(&big_kernel_lock);
        while (sched_finish_flag != PLAT_CPU_NUM)
                ;
        /* ============ Finish Barrier ============ */

        /* check each queue shoule be empty */
        BUG_ON(!list_empty(&(rr_ready_queue_meta[cpuid].queue_head)));
}

void tst_pbrr(void)
{
        int cpuid = 0;
        struct thread *thread = 0;

        cpuid = smp_get_cpu_id();

        /* ============ Start Barrier ============ */
        lock(&big_kernel_lock);
        sched_start_flag++;
        unlock(&big_kernel_lock);
        while (sched_start_flag != PLAT_CPU_NUM)
                ;
        /* ============ Start Barrier ============ */

        int i = 0, j = 0, k = 0, prio = 0;
        struct thread_ctx *thread_ctx = NULL;

        /* Init threads */
        for (i = 0; i < THREAD_NUM; i++) {
                for (j = 0; j < PLAT_CPU_NUM; j++) {
                        prio = ((i + j) % (MAX_PRIO - 1)) + 1;

                        thread = obj_alloc(TYPE_THREAD, sizeof(*thread));
                        obj_ref(thread);
                        BUG_ON(!(thread->thread_ctx =
                                         create_thread_ctx(TYPE_TESTS)));
                        init_thread_ctx(thread, 0, 0, prio, TYPE_TESTS, j);
                        for (k = 0; k < REG_NUM; k++)
                                thread->thread_ctx->ec.reg[k] = prio;
                        BUG_ON(pb_sched_enqueue(thread));
                }
        }

        for (j = 0; j < TEST_NUM; j++) {
                /* Each core try to get those threads */
                for (i = 0; i < THREAD_NUM * PLAT_CPU_NUM; i++) {
                        /* get thread and dequeue from ready queue */
                        do {
                                /* do it again if choose idle thread */
                                pbrr_sched();
                                BUG_ON(!current_thread);
                                BUG_ON(!current_thread->thread_ctx);
                                current_thread->thread_ctx->sc->budget = 0;

                                if (is_valid_prev_thread(
                                            current_thread->prev_thread))
                                        current_thread->prev_thread->thread_ctx
                                                ->kernel_stack_state = KS_FREE;
                        } while (current_thread->thread_ctx->type == TYPE_IDLE);
                        BUG_ON(!current_thread->thread_ctx->sc);
                        /* Current thread set affinitiy */
                        current_thread->thread_ctx->affinity =
                                (i + cpuid) % PLAT_CPU_NUM;
                        thread_ctx = (struct thread_ctx *)switch_context();

                        /*
                         * It should be mentioned that sched() will go through
                         * the FPU save/restore procedure which might modify the
                         * value of sstatus register and result in the failure
                         * of this test. Fortunately, in this test the sstatus
                         * register will remain the same. But future development
                         * must bear this in mind.
                         */
                        for (k = 0; k < REG_NUM; k++)
                                BUG_ON(thread_ctx->ec.reg[k]
                                       != thread_ctx->sc->prio);
                }
        }

        for (i = 0; i < THREAD_NUM * PLAT_CPU_NUM; i++) {
                /* get thread and dequeue from ready queue */
                do {
                        /* do it again if choose idle thread */
                        pbrr_sched();
                        BUG_ON(!current_thread);
                        BUG_ON(!current_thread->thread_ctx);
                        current_thread->thread_ctx->sc->budget = 0;

                        if (is_valid_prev_thread(current_thread->prev_thread))
                                current_thread->prev_thread->thread_ctx
                                        ->kernel_stack_state = KS_FREE;
                } while (current_thread->thread_ctx->type == TYPE_IDLE);
                BUG_ON(!current_thread->thread_ctx->sc);
                destroy_thread_ctx(current_thread);
                kfree(container_of(current_thread, struct object, opaque));
                current_thread = NULL;
        }

        /* ============ Finish Barrier ============ */
        lock(&big_kernel_lock);
        sched_finish_flag++;
        unlock(&big_kernel_lock);
        while (sched_finish_flag != PLAT_CPU_NUM)
                ;
        /* ============ Finish Barrier ============ */

        /* check each queue shoule be empty */
}
