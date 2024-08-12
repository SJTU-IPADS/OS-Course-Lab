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

#include <mm/mm.h>
#include <machine.h>
#include <sched/sched.h>
#include <object/thread.h>
#include <mm/uaccess.h>
#include <arch/machine/smp.h>
#include <arch/trustzone/smc.h>
#include <arch/trustzone/tlogger.h>

struct smc_percpu_struct {
        struct thread *waiting_thread;
        struct smc_registers regs_k;
        struct smc_registers *regs_u;
} smc_percpu_structs[PLAT_CPU_NUM];

void smc_init(void)
{
        u32 cpuid;
        struct smc_percpu_struct *percpu;

        for (cpuid = 0; cpuid < PLAT_CPU_NUM; cpuid++) {
                percpu = &smc_percpu_structs[cpuid];
                percpu->waiting_thread = NULL;
        }
}

static kernel_shared_varibles_t kernel_var;

/* Yield smc entry. Register x0, x1, x2, x3, x4 */
void handle_yield_smc(unsigned long x0, unsigned long x1, unsigned long x2,
                      unsigned long x3, unsigned long x4)
{
        int ret;
        struct smc_percpu_struct *percpu;

        enable_tlogger();

        /*
         * Protocol with tzdriver to identify the first smc request. gtask will
         * get these registers soon through `ot_sys_tee_pull_kernel_var`.
         */
        if (x2 == 0xf) {
                kernel_var.params_stack[0] = x0;
                kernel_var.params_stack[1] = x1;
                kernel_var.params_stack[2] = x2;
                kernel_var.params_stack[3] = x3;
                kernel_var.params_stack[4] = x4;
        }

        /* store register into per-cpu structure */
        percpu = &smc_percpu_structs[smp_get_cpu_id()];
        percpu->regs_k.x0 = TZ_SWITCH_REQ_STD_REQUEST;
        percpu->regs_k.x1 = x1;
        percpu->regs_k.x2 = x2;
        percpu->regs_k.x3 = x3;
        percpu->regs_k.x4 = x4;

        if (percpu->waiting_thread) {
                /* copy registers to waiting thread(smc thread) */
                struct thread *current = current_thread;
                switch_thread_vmspace_to(percpu->waiting_thread);
                ret = copy_to_user((char *)percpu->regs_u,
                                   (char *)&percpu->regs_k,
                                   sizeof(percpu->regs_k));
                switch_thread_vmspace_to(current);
                arch_set_thread_return(percpu->waiting_thread, ret);
                BUG_ON(sched_enqueue(percpu->waiting_thread));
                percpu->waiting_thread = NULL;
        } else {
                BUG("SMC thread is not waiting for smc request.\n");
        }

        sched();
        eret_to_thread(switch_context());
}

int ot_sys_tee_wait_switch_req(struct smc_registers *regs_u)
{
        int ret;
        struct smc_percpu_struct *percpu;

        percpu = &smc_percpu_structs[smp_get_cpu_id()];

        if (percpu->waiting_thread) {
                kwarn("Some other thread is waiting for smc request.\n");
                return -EINVAL;
        }

        percpu->waiting_thread = current_thread;
        percpu->regs_u = regs_u;

        thread_set_ts_blocking(current_thread);

        sched();
        eret_to_thread(switch_context());
        BUG("Should not reach here.\n");
}

int ot_sys_tee_switch_req(struct smc_registers *regs_u)
{
        int ret;
        struct smc_registers regs_k;

        ret = copy_from_user(&regs_k, regs_u, sizeof(regs_k));
        kinfo("%s %d\n", __func__, __LINE__);
        if (ret < 0) {
                return ret;
        }

        if (regs_k.x0 == TZ_SWITCH_REQ_ENTRY_DONE) {
                /* the first smc in trustzone to tell el3 the address of
                 * tz_vectors */
                regs_k.x0 = SMC_ENTRY_DONE;
                regs_k.x1 = (vaddr_t)&tz_vectors;
        } else if (regs_k.x0 == TZ_SWITCH_REQ_STD_RESPONSE) {
                regs_k.x0 = SMC_STD_RESPONSE;
                regs_k.x1 = SMC_EXIT_NORMAL;
        } else {
                return -1;
        }

        arch_set_thread_return(current_thread, 0);
        smc_call(regs_k.x0, regs_k.x1, regs_k.x2, regs_k.x3, regs_k.x4);
        BUG("Should not reach here.\n");
}

int ot_sys_tee_pull_kernel_var(kernel_shared_varibles_t *kernel_var_ubuf)
{
        int ret;

        kinfo("%s\n", __func__);

        if (check_user_addr_range((vaddr_t)kernel_var_ubuf,
                                  sizeof(kernel_shared_varibles_t))) {
                return -EINVAL;
        }

        ret = copy_to_user(
                kernel_var_ubuf, &kernel_var, sizeof(kernel_shared_varibles_t));

        return ret;
}
