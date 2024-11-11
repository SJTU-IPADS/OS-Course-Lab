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

#include <uapi/get_system_info.h>
#include <object/cap_group.h>
#include <object/thread.h>
#include <mm/uaccess.h>
#include <common/errno.h>
#include <common/util.h>
#include <irq/irq.h>

/* Just for Debug & Monitor Util */

static int get_process_info(void *ubuffer, unsigned long size,
                            cap_t cap_group_cap)
{
        struct process_info pbuffer;
        struct cap_group *cap_group;
        struct vmspace *vmspace;
        struct vmregion *vmr;
        int ret = 0;

        if (size < sizeof(struct process_info)) {
                ret = -EINVAL;
                goto out;
        }

        cap_group = (struct cap_group *)obj_get(current_cap_group,
                                                (cap_t)cap_group_cap,
                                                TYPE_CAP_GROUP);
        if (!cap_group) {
                ret = -ECAPBILITY;
                goto out;
        }
        
        vmspace = (struct vmspace *)obj_get(cap_group,
                                            VMSPACE_OBJ_ID,
                                            TYPE_VMSPACE);
        if (!vmspace) {
                ret = -ECAPBILITY;
                goto free_cap_group;
        }

        memcpy(pbuffer.cap_group_name, cap_group->cap_group_name,
               sizeof(cap_group->cap_group_name));
        pbuffer.thread_num = cap_group->thread_cnt;

        lock(&vmspace->pgtbl_lock);
        pbuffer.rss = vmspace->rss;
        unlock(&vmspace->pgtbl_lock);

        pbuffer.vss = 0;
        lock(&vmspace->vmspace_lock);
        for_each_in_list (vmr,
                          struct vmregion,
                          list_node,
                          &(vmspace->vmr_list)) {
                pbuffer.vss += vmr->size;
        }
        unlock(&vmspace->vmspace_lock);
        
        copy_to_user(ubuffer, (void *)(&pbuffer),
                     sizeof(struct process_info));

        obj_put(vmspace);
free_cap_group:
        obj_put(cap_group);
out:
        return ret;
}

static int get_thread_info(void *ubuffer, unsigned long size,
                           cap_t cap_group_cap)
{
        struct thread_info *tbuffer;
        struct cap_group *cap_group;
        struct thread *thread;
        int i = 0, ret = 0, thread_num;

        cap_group = (struct cap_group *)obj_get(current_cap_group,
                                                (cap_t)cap_group_cap,
                                                TYPE_CAP_GROUP);
        if (!cap_group) {
                ret = -ECAPBILITY;
                goto out;
        }
        if (size < sizeof(struct thread_info) * cap_group->thread_cnt) {
                ret = -EINVAL;
                goto free_cap_group;
        }
        thread_num = cap_group->thread_cnt;
        tbuffer = (struct thread_info *)kmalloc(sizeof(struct thread_info) *
                                                thread_num);
        if (!tbuffer) {
                ret = -ENOMEM;
                goto free_cap_group;
        }

        for_each_in_list (thread,
                          struct thread,
                          node,
                          &(cap_group->thread_list)) {
                tbuffer[i].type = thread->thread_ctx->type;
                tbuffer[i].state = thread->thread_ctx->state;
                tbuffer[i].cpuid = thread->thread_ctx->cpuid;
                tbuffer[i].affinity = thread->thread_ctx->affinity;
                tbuffer[i].prio = thread->thread_ctx->sc ?
                                  thread->thread_ctx->sc->prio : -1;
                i++;

                /*
                 * This is necessary since the length of
                 * `cap_group->thread_list` may not be equal to
                 * the `cap_group->thread_cnt`.
                 */
                if (i == thread_num) {
                        break;
                }
        }
        copy_to_user(ubuffer, (void *)tbuffer,
                     sizeof(struct thread_info) * thread_num);
        kfree(tbuffer);
free_cap_group:
        obj_put(cap_group);
out:
        return ret;
}

int sys_get_system_info(enum gsi_op op, void *ubuffer,
                        unsigned long size, long arg)
{
        int ret = 0;

        switch (op) {
        case GSI_IRQ:
#ifdef CHCORE_ARCH_SPARC
                ret = plat_get_irq_info(ubuffer, size);
#endif
                break;
        case GSI_PROCESS:
                ret = get_process_info(ubuffer, size, (cap_t)arg);
                break;
        case GSI_THREAD: {
                ret = get_thread_info(ubuffer, size, (cap_t)arg);
                break;
        }
        default:
                ret = -EINVAL;
                break;
        }

        return ret;
}
