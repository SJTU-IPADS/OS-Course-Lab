/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#include <object/ptrace.h>
#include <mm/uaccess.h>
#include <lib/printk.h>
#include <common/list.h>
#include <arch/machine/registers.h>
#include <ipc/notification.h>

#define BKPT_WATING   0 /* if ptrace is waiting for breakpoint */
#define BKPT_SIGNALED 1 /* if the breakpoint is triggered */

#define MAX_THREADS 128

#define SUSPEND   false
#define UNSUSPEND true

struct ptrace {
        struct cap_group *ptrace_proc;

        /* records if the tracee has triggered breakpoints or is waiting */
        int bkpt_status;

        /* record each threads breakpoint status */
        volatile unsigned long bkpt_bmp[BITS_TO_LONGS(MAX_THREADS)];

        /*
         * The thread calling CHCORE_PTRACE_SINGLESTEP requst will wait on
         * the notification.
         */
        struct notification singlestep_notifc;
};

int ptrace_init(struct ptrace *ptrace, struct thread *thread)
{
        ptrace->ptrace_proc = thread->cap_group;
        ptrace->ptrace_proc->attached_ptrace = ptrace;
        ptrace->bkpt_status = BKPT_WATING;
        return init_notific(&ptrace->singlestep_notifc);
}

CAP_ALLOC_IMPL(create_ptrace,
               TYPE_PTRACE,
               sizeof(struct ptrace),
               CAP_ALLOC_OBJ_FUNC(ptrace_init, thread),
               CAP_ALLOC_OBJ_FUNC(ptrace_deinit),
               struct thread *thread);

static int create_ptrace(int main_thread_cap)
{
        struct thread *thread;
        cap_t ret;

        thread = obj_get(current_cap_group, main_thread_cap, TYPE_THREAD);
        if (thread == NULL) {
                ret = -ECAPBILITY;
                goto out_fail_get_thread;
        }


        ret = CAP_ALLOC_CALL(
                create_ptrace, current_cap_group, CAP_RIGHT_NO_RIGHTS, thread);

        obj_put(thread);

out_fail_get_thread:
        return ret;
}

/* Get the number of threads */
static int ptrace_getthreads(struct ptrace *ptrace)
{
        struct thread *thread;
        int ret = 0;

        lock(&ptrace->ptrace_proc->threads_lock);
        for_each_in_list (thread,
                          struct thread,
                          node,
                          &(ptrace->ptrace_proc->thread_list)) {
                ret++;
        }
        unlock(&ptrace->ptrace_proc->threads_lock);

        return ret;
}

/*
 * Change tracee's threads' suspend states
 * @runnable: suppend all threads (false) or resume all threads' execution
 * (true).
 */
static int ptrace_change_threads_suspend(struct ptrace *ptrace, bool runnable)
{
        struct thread *thread;

        /* reset breakpoint status before running */
        if (runnable)
                ptrace->bkpt_status = BKPT_WATING;

        lock(&ptrace->ptrace_proc->threads_lock);
        for_each_in_list (thread,
                          struct thread,
                          node,
                          &(ptrace->ptrace_proc->thread_list)) {
                if (runnable) {
                        thread_set_resume(thread);
                } else {
                        thread_set_suspend(thread);
                }
        }
        unlock(&ptrace->ptrace_proc->threads_lock);

        return 0;
}

static struct thread *find_thread_by_tid(struct ptrace *ptrace,
                                         unsigned long tid)
{
        struct thread *thread = NULL, *cur_thread;
        int cur_tid = 1; /* tid starts from 1 */

        if (tid > MAX_THREADS)
                return NULL;

        lock(&ptrace->ptrace_proc->threads_lock);
        for_each_in_list (cur_thread,
                          struct thread,
                          node,
                          &(ptrace->ptrace_proc->thread_list)) {
                if (cur_tid == tid) {
                        thread = cur_thread;
                        break;
                }
                cur_tid++;
        }
        unlock(&ptrace->ptrace_proc->threads_lock);

        return thread;
}

/* Get all registers from thread @tid and write to the buffer @data */
static int ptrace_getregs(struct ptrace *ptrace, unsigned long tid, void *data)
{
        struct thread *thread = NULL;
        struct chcore_user_regs_struct user_regs;

        thread = find_thread_by_tid(ptrace, tid);
        if (!thread)
                return -EINVAL;

        arch_ptrace_getregs(thread, &user_regs);

        return copy_to_user(data, (char *)&user_regs, sizeof(user_regs));
}

/*
 * Get memory (base address @addr with length of sizeof(long)) from thread @tid.
 * And store the data to the buffer @data
 */
static long ptrace_peekdata(struct ptrace *ptrace, unsigned long tid,
                            void *addr, void *data)
{
        struct thread *thread = NULL;
        unsigned long val = 0;
        int ret, copy_ret;

        thread = find_thread_by_tid(ptrace, tid);
        if (!thread)
                return -EINVAL;

        copy_ret = copy_from_user_remote(
                thread->cap_group, (char *)&val, addr, sizeof(long));
        if (copy_ret)
                val = 0;

        ret = copy_to_user(data, (char *)&val, sizeof(long));

        return copy_ret ? copy_ret : ret;
}

/*
 * Store @data to memory (base address @addr with length of sizeof(long))
 * of thread @tid.
 */
static long ptrace_pokedata(struct ptrace *ptrace, unsigned long tid,
                            void *addr, void *data)
{
        struct thread *thread = NULL;
        unsigned long val = (unsigned long)data;

        thread = find_thread_by_tid(ptrace, tid);
        if (!thread)
                return -EINVAL;

        return copy_to_user_remote(
                thread->cap_group, addr, (char *)&val, sizeof(long));
}

/*
 * Get register (at address @addr, which is architecture-specific) from thread
 * @tid. And store the data to the buffer @data
 */
static long ptrace_peekuser(struct ptrace *ptrace, unsigned long tid,
                            void *addr, void *data)
{
        struct thread *thread = NULL;
        unsigned long val = 0;
        int ret;

        thread = find_thread_by_tid(ptrace, tid);
        if (!thread)
                return -EINVAL;

        ret = arch_ptrace_getreg(thread, (unsigned long)addr, &val);
        if (ret)
                return ret;

        ret = copy_to_user(data, (char *)&val, sizeof(long));
        if (ret)
                return ret;

        return 0;
}

/*
 * Store @data to register (at address @addr, which is architecture-specific)
 * of thread @tid.
 */
static long ptrace_pokeuser(struct ptrace *ptrace, unsigned long tid,
                            void *addr, void *data)
{
        struct thread *thread = NULL;

        thread = find_thread_by_tid(ptrace, tid);
        if (!thread)
                return -EINVAL;

        return arch_ptrace_setreg(
                thread, (unsigned long)addr, (unsigned long)data);
}

/* Currently wait unblockingly */
static int ptrace_waitbkpt(struct ptrace *ptrace)
{
        if (ptrace->bkpt_status == BKPT_WATING)
                return -EAGAIN;

        return 0;
}

/*
 * Get the breakpoint status of a thread.
 * @tid: the thread to query.
 * @return: true if the thread stops at a breakpoint, false otherwise.
 */
static int ptrace_getbkptinfo(struct ptrace *ptrace, unsigned long tid)
{
        if (tid > MAX_THREADS)
                return -EINVAL;

        /* tid starts from 1, index in bkpt_bmp starts from 0 */
        return get_bit(tid - 1, ptrace->bkpt_bmp);
}

void ptrace_deinit(void *ptrace_ptr)
{
        struct ptrace *ptrace = ptrace_ptr;

        ptrace->ptrace_proc->attached_ptrace = NULL;
        ptrace_change_threads_suspend(ptrace, UNSUSPEND);
        notification_deinit((void *)&ptrace->singlestep_notifc);
}

static int ptrace_detach(struct ptrace *ptrace)
{
        ptrace_deinit(ptrace);
        return 0;
}

/* Execute the thread @tid in singlestep mode */
static int ptrace_singlestep(struct ptrace *ptrace, unsigned long tid)
{
        struct thread *thread = NULL;
        int ret;

        thread = find_thread_by_tid(ptrace, tid);
        if (!thread)
                return -EINVAL;

        if (!thread_is_suspend(thread)) {
                kwarn("%s on a running thread\n", __func__);
                return -EINVAL;
        }

        ret = arch_set_thread_singlestep(thread, true);
        if (ret) {
                kwarn("%s arch singlestep error\n", __func__);
                return ret;
        }

        thread_set_resume(thread);

        arch_set_thread_return(current_thread, 0);
        wait_notific(&ptrace->singlestep_notifc, true, NULL);
        /* Never returns */

        return 0;
}

static int ptrace_ops(int req, unsigned long ptrace_cap, unsigned long tid,
                      void *addr, void *data)
{
        struct ptrace *ptrace;
        int ret;

        ptrace = obj_get(current_cap_group, ptrace_cap, TYPE_PTRACE);
        if (ptrace->ptrace_proc == NULL) {
                ret = -ECAPBILITY;
                goto out;
        }

        switch (req) {
        case CHCORE_PTRACE_GETTHREADS:
                ret = ptrace_getthreads(ptrace);
                break;
        case CHCORE_PTRACE_SUSPEND:
                ret = ptrace_change_threads_suspend(ptrace, SUSPEND);
                break;
        case CHCORE_PTRACE_CONT:
                ret = ptrace_change_threads_suspend(ptrace, UNSUSPEND);
                break;
        case CHCORE_PTRACE_GETREGS:
                ret = ptrace_getregs(ptrace, tid, data);
                break;
        case CHCORE_PTRACE_PEEKDATA:
                ret = ptrace_peekdata(ptrace, tid, addr, data);
                break;
        case CHCORE_PTRACE_POKEDATA:
                ret = ptrace_pokedata(ptrace, tid, addr, data);
                break;
        case CHCORE_PTRACE_PEEKUSER:
                ret = ptrace_peekuser(ptrace, tid, addr, data);
                break;
        case CHCORE_PTRACE_POKEUSER:
                ret = ptrace_pokeuser(ptrace, tid, addr, data);
                break;
        case CHCORE_PTRACE_WAITBKPT:
                ret = ptrace_waitbkpt(ptrace);
                break;
        case CHCORE_PTRACE_SINGLESTEP:
                ret = ptrace_singlestep(ptrace, tid);
                break;
        case CHCORE_PTRACE_GETBKPTINFO:
                ret = ptrace_getbkptinfo(ptrace, tid);
                break;
        case CHCORE_PTRACE_DETACH:
                ret = ptrace_detach(ptrace);
                break;
        default:
                ret = -EINVAL;
        }

        obj_put(ptrace);
out:
        return ret;
}

int sys_ptrace(int req, unsigned long cap, unsigned long tid, void *addr,
               void *data)
{
        if (req == CHCORE_PTRACE_ATTACH)
                return create_ptrace((cap_t)cap);
        else
                return ptrace_ops(req, (cap_t)cap, tid, addr, data);
}

/*
 * The generic hook to handle breakpoint.
 * Should be called by each architecture when a breakpoint is triggered.
 *
 * FIXME: The tid record could be wrong if new thread are created before all
 *	  threads stop execution
 */
void handle_breakpoint(void)
{
        struct ptrace *ptrace;
        struct thread *thread;
        int tid = 0; /* tid in bkpt_bmp starts from 0 */

        ptrace = current_cap_group->attached_ptrace;
        if (!ptrace) {
                kwarn("breakpoint not handled\n");
                return;
        }

        ptrace_change_threads_suspend(ptrace, SUSPEND);

        lock(&ptrace->ptrace_proc->threads_lock);
        for_each_in_list (thread,
                          struct thread,
                          node,
                          &(ptrace->ptrace_proc->thread_list)) {
                if (thread == current_thread)
                        break;
                tid++;
        }
        unlock(&ptrace->ptrace_proc->threads_lock);

        BUG_ON(thread != current_thread);

        ptrace->bkpt_status = BKPT_SIGNALED;

        if (tid >= MAX_THREADS) {
                kwarn("%s exceeding max threads\n", __func__);
                return;
        }

        set_bit(tid, ptrace->bkpt_bmp);

        sched();
        eret_to_thread(switch_context());
}

void handle_singlestep(void)
{
        struct ptrace *ptrace;

        ptrace = current_cap_group->attached_ptrace;
        if (!ptrace) {
                kwarn("singlestep not handled\n");
                return;
        }

        BUG_ON(ptrace->singlestep_notifc.waiting_threads_count != 1);
        signal_notific(&ptrace->singlestep_notifc);

        arch_set_thread_singlestep(current_thread, false);

        thread_set_suspend(current_thread);
        sched();
        eret_to_thread(switch_context());
}
