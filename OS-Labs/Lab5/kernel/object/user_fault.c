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

#include <arch/mmu.h>
#include <arch/sync.h>
#include <mm/cache.h>
#include <ipc/notification.h>
#include <irq/irq.h>
#include <common/errno.h>
#include <common/radix.h>
#include <common/lock.h>
#include <common/types.h>
#include <sched/sched.h>
#include <mm/kmalloc.h>
#include <lib/printk.h>
#include <mm/vmspace.h>
#include <mm/kmalloc.h>
#include <mm/mm.h>
#include <object/object.h>
#include <object/user_fault.h>
#include <lib/ring_buffer.h>

struct lock fmap_fault_pool_list_lock;
struct list_head fmap_fault_pool_list;

static void user_fault_init(void)
{
        static int inited = 0;

        if (!atomic_cmpxchg_32(&inited, 0, 1)) {
                lock_init(&fmap_fault_pool_list_lock);
                init_list_head(&fmap_fault_pool_list);
        }
}

static struct fmap_fault_pool *get_current_fault_pool(void)
{
        badge_t badge;
        struct fmap_fault_pool *pool_iter;

        badge = current_cap_group->badge;
        for_each_in_list (pool_iter,
                          struct fmap_fault_pool,
                          node,
                          &fmap_fault_pool_list) {
                if (pool_iter->cap_group_badge == badge) {
                        return pool_iter;
                }
        }

        return NULL;
}

static struct fault_pending_thread *
get_current_pending_thread(badge_t client_badge, vaddr_t fault_va)
{
        struct fault_pending_thread *pt;
        struct fmap_fault_pool *pool;

        pool = get_current_fault_pool();
        if (!pool)
                return NULL;

        for_each_in_list (
                pt, struct fault_pending_thread, node, &pool->pending_threads) {
                if (pt->fault_badge == client_badge
                    && pt->fault_va == fault_va) {
                        return pt;
                }
        }
        return NULL;
}

/* syscall */
int sys_user_fault_register(cap_t notific_cap, vaddr_t msg_buffer)
{
        int ret;
        struct notification *notific;
        struct ring_buffer *msg_buffer_kva;
        /* *msg_buffer_kva points to the virtual address of a ring buffer
         * struct, so no need to initialize */
        badge_t badge;
        struct fmap_fault_pool *pool_iter;

        user_fault_init();

        badge = current_cap_group->badge;

        /* Validate arguments */
        notific = obj_get(current_cap_group, notific_cap, TYPE_NOTIFICATION);
        if (notific == NULL) {
                return -EINVAL;
        }

        ret = trans_uva_to_kva(msg_buffer, (vaddr_t *)&msg_buffer_kva);
        if (ret != 0) {
                return -EINVAL;
        }

        lock(&fmap_fault_pool_list_lock);
        if (get_current_fault_pool() != NULL) {
                /* pool already exists */
                unlock(&fmap_fault_pool_list_lock);
                return -EINVAL;
        }

        /* Create a fmap_fault_pool and add to list */
        pool_iter = (struct fmap_fault_pool *)kmalloc(sizeof(*pool_iter));
        if (!pool_iter) {
                unlock(&fmap_fault_pool_list_lock);
                return -ENOMEM;
        }

        pool_iter->cap_group_badge = badge;
        pool_iter->notific = notific;
        pool_iter->msg_buffer_kva = msg_buffer_kva;
        lock_init(&pool_iter->lock);
        init_list_head(&pool_iter->pending_threads);

        list_append(&pool_iter->node, &fmap_fault_pool_list);
        unlock(&fmap_fault_pool_list_lock);

        return 0;
}

int sys_user_fault_map(badge_t client_badge, vaddr_t fault_va, vaddr_t remap_va,
                       bool copy, unsigned long perm)
{
        struct fmap_fault_pool *current_pool;
        struct fault_pending_thread *pending_thread;
        struct thread *thread_to_wake;
        struct vmspace *handler_vmspace;
        struct vmspace *fault_vmspace;
        struct vmregion *fault_vmr;
        struct pmobject *fault_pmo;
        paddr_t pa, new_pa;
        void *new_page;
        int ret;
        bool page_allocated = false;
        long rss = 0;

        current_pool = get_current_fault_pool();

        if (!current_pool) {
                return -EINVAL;
        }

        fault_va = ROUND_DOWN(fault_va, PAGE_SIZE);
        remap_va = ROUND_DOWN(remap_va, PAGE_SIZE);

        /* Find corresponding pending thread */
        lock(&current_pool->lock);
        pending_thread = get_current_pending_thread(client_badge, fault_va);
        if (!pending_thread) {
                unlock(&current_pool->lock);
                return -EINVAL;
        }
        list_del(&pending_thread->node);
        unlock(&current_pool->lock);

        thread_to_wake = pending_thread->thread;
        kfree(pending_thread);

        /* Get handler space va, which page will be mapped in fault va */
        if (remap_va) {
                handler_vmspace = obj_get(
                        current_cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
                if (handler_vmspace == NULL) {
                        return -EINVAL;
                }
                lock(&handler_vmspace->pgtbl_lock);
                ret = query_in_pgtbl(
                        handler_vmspace->pgtbl, remap_va, &pa, NULL);
                if (ret) {
                        /* remap_va is not mapped in handler_vmspace */
                        unlock(&handler_vmspace->pgtbl_lock);
                        obj_put(handler_vmspace);
                        return -EINVAL;
                }
                unlock(&handler_vmspace->pgtbl_lock);
                obj_put(handler_vmspace);
        }

        /* Decide whether copy the physical page or share */
        if (!copy) {
                if (!remap_va)
                        return -EINVAL;
                new_pa = pa;
        } else {
                new_page = get_pages(0);
                if (new_page == NULL)
                        return -EINVAL;
                if (remap_va)
                        memcpy(new_page, (void *)phys_to_virt(pa), PAGE_SIZE);
                else
                        memset(new_page, 0, PAGE_SIZE);
                new_pa = (paddr_t)virt_to_phys(new_page);
                page_allocated = true;
        }

        /* Fill fault pa with target page's pa */
        fault_vmspace = obj_get(
                thread_to_wake->cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
        if (fault_vmspace == NULL) {
                return -EINVAL;
        }
        if (page_allocated) {
                lock(&fault_vmspace->vmspace_lock);
                fault_vmr = find_vmr_for_va(fault_vmspace, fault_va);
                if (fault_vmr == NULL) {
                        unlock(&fault_vmspace->vmspace_lock);
                        obj_put(fault_vmspace);
                        return -EINVAL;
                }
                fault_pmo = fault_vmr->pmo;
                commit_page_to_pmo(fault_pmo, new_pa, new_pa);
        }

        lock(&fault_vmspace->pgtbl_lock);
        ret = map_range_in_pgtbl(
                fault_vmspace->pgtbl, fault_va, new_pa, PAGE_SIZE, perm, &rss);
        fault_vmspace->rss += rss;
        BUG_ON(ret);
        unlock(&fault_vmspace->pgtbl_lock);

        if (page_allocated) {
                unlock(&fault_vmspace->vmspace_lock);
        }

        obj_put(fault_vmspace);

        switch_thread_vmspace_to(thread_to_wake);
        if (perm & VMR_EXEC) {
                arch_flush_cache(fault_va, PAGE_SIZE, SYNC_IDCACHE);
        }
        switch_thread_vmspace_to(current_thread);

        /* Pending thread should come back to scheduler */
        BUG_ON(sched_enqueue(thread_to_wake));

        return 0;
}

/* Only for Lab7. Enqueue pending thread only if completed=true */
int sys_user_fault_map_batched(badge_t client_badge, vaddr_t fault_va, vaddr_t remap_va,
        bool copy, unsigned long perm, bool completed, vaddr_t orig_fault_va)
{
        struct fmap_fault_pool *current_pool;
        struct fault_pending_thread *pending_thread;
        struct thread *thread_to_wake;
        struct vmspace *handler_vmspace;
        struct vmspace *fault_vmspace;
        struct vmregion *fault_vmr;
        struct pmobject *fault_pmo;
        paddr_t pa, new_pa;
        void *new_page;
        int ret;
        bool page_allocated = false;
        long rss = 0;

        struct llm_page *llm_page;
        bool is_same_llm_page_found = false;

        current_pool = get_current_fault_pool();

        if (!current_pool) {
                return -EINVAL;
        }

        fault_va = ROUND_DOWN(fault_va, PAGE_SIZE);
        remap_va = ROUND_DOWN(remap_va, PAGE_SIZE);

        /* Find corresponding pending thread */
        lock(&current_pool->lock);
        pending_thread = get_current_pending_thread(client_badge, orig_fault_va);
        if (!pending_thread) {
                unlock(&current_pool->lock);
                return -EINVAL;
        }
        if (completed)
                list_del(&pending_thread->node);
        unlock(&current_pool->lock);

        thread_to_wake = pending_thread->thread;
        if (completed)
                kfree(pending_thread);

        /* Get handler space va, which page will be mapped in fault va */
        if (remap_va) {
                handler_vmspace = obj_get(
                        current_cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
                if (handler_vmspace == NULL) {
                        return -EINVAL;
                }
                lock(&handler_vmspace->pgtbl_lock);
                ret = query_in_pgtbl(
                        handler_vmspace->pgtbl, remap_va, &pa, NULL);
                if (ret) {
                        /* remap_va is not mapped in handler_vmspace */
                        unlock(&handler_vmspace->pgtbl_lock);
                        obj_put(handler_vmspace);
                        return -EINVAL;
                }
                unlock(&handler_vmspace->pgtbl_lock);
                obj_put(handler_vmspace);
        }

        /* Decide whether copy the physical page or share */
        if (!copy) {
                if (!remap_va)
                        return -EINVAL;
                new_pa = pa;
        } else {
                new_page = get_pages(0);
                if (new_page == NULL)
                        return -EINVAL;
                if (remap_va)
                        memcpy(new_page, (void *)phys_to_virt(pa), PAGE_SIZE);
                else
                        memset(new_page, 0, PAGE_SIZE);
                new_pa = (paddr_t)virt_to_phys(new_page);
                page_allocated = true;
        }

        /* Fill fault pa with target page's pa */
        fault_vmspace = obj_get(
                thread_to_wake->cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
        if (fault_vmspace == NULL) {
                return -EINVAL;
        }
        lock(&fault_vmspace->vmspace_lock);
        fault_vmr = find_vmr_for_va(fault_vmspace, fault_va);
        if (fault_vmr == NULL) {
                unlock(&fault_vmspace->vmspace_lock);
                obj_put(fault_vmspace);
                return -EINVAL;
        }
        if (page_allocated) {
                fault_pmo = fault_vmr->pmo;
                commit_page_to_pmo(fault_pmo, new_pa, new_pa);
        }
        
        for_each_in_list(llm_page, struct llm_page, node, &fault_vmr->llm_pages) {
                if (llm_page->vaddr == fault_va) {
                        is_same_llm_page_found = true;
                        break;
                }
        }
        /* if fault_va already in lru list, move it to the end 
           else, allocate a new llm_page and append to lru list */
        if (is_same_llm_page_found) {
                list_del(&llm_page->node);
        } else {
                llm_page = (struct llm_page *)kmalloc(sizeof(*llm_page));
                if (!llm_page) {
                        unlock(&fault_vmspace->vmspace_lock);
                        obj_put(fault_vmspace);
                        return -ENOMEM;
                }
                llm_page->vaddr = fault_va;
        }
        list_append(&llm_page->node, &fault_vmr->llm_pages);

        lock(&fault_vmspace->pgtbl_lock);
        ret = map_range_in_pgtbl(
                fault_vmspace->pgtbl, fault_va, new_pa, PAGE_SIZE, perm, &rss);
        fault_vmspace->rss += rss;
        BUG_ON(ret);
        
        /* if fault_va is new, increment num_llm_pages, 
           and unmap the least recently mapped page if list is full */
        if (!is_same_llm_page_found) {
                if (fault_vmr->num_llm_pages == MAX_LLM_PAGE_NUM) {
                        // printk("unmap 0x%lx\n", llm_page->vaddr);
                        llm_page = container_of(fault_vmr->llm_pages.next, struct llm_page, node);
                        rss = 0;
                        ret = unmap_range_in_pgtbl(fault_vmspace->pgtbl, llm_page->vaddr, PAGE_SIZE, &rss);
                        fault_vmspace->rss += rss;
                        BUG_ON(ret);
                        list_del(&llm_page->node);
                        kfree(llm_page);
                } else {
                        fault_vmr->num_llm_pages++;
                }
        }
        unlock(&fault_vmspace->pgtbl_lock);
        
        unlock(&fault_vmspace->vmspace_lock);        
        obj_put(fault_vmspace);

        switch_thread_vmspace_to(thread_to_wake);
        if (perm & VMR_EXEC) {
                arch_flush_cache(fault_va, PAGE_SIZE, SYNC_IDCACHE);
        }
        switch_thread_vmspace_to(current_thread);

        if (completed)
                /* Pending thread should come back to scheduler */
                /* wake up pending thread only if prefetching is completed */
                BUG_ON(sched_enqueue(thread_to_wake));

        return 0;
}

/* Handling a user page fault */
void handle_user_fault(struct pmobject *pmo, vaddr_t fault_va)
{
        struct fmap_fault_pool *fault_pool;
        struct fault_pending_thread *pending_thread;
        int ret;

        fault_pool = (struct fmap_fault_pool *)pmo->private;
        kdebug("pmo file fault: badge=%x, va=%lx\n",
               fault_pool->cap_group_badge,
               fault_va);

        
        /* Lab7 test, track page faults for specified pmo */
        if (pmo->page_faults >= 0) {
                pmo->page_faults++;
        }
        /*
         * Fault thread should pending until user handling finished.
         * Record (fault_badge, fault_va) -> thread here.
         */
        pending_thread =
                (struct fault_pending_thread *)kmalloc(sizeof(*pending_thread));
        if (!pending_thread) {
                BUG_ON(1);
        }

        pending_thread->fault_badge = current_cap_group->badge;
        pending_thread->fault_va = fault_va;
        pending_thread->thread = current_thread;

        /* The fault_pool lock also protect producer ptr racing */
        lock(&fault_pool->lock);

        if (if_buffer_full(fault_pool->msg_buffer_kva)) {
                BUG_ON(1);
        } else {
                /* successfully fetch slot from server space */
                struct user_fault_msg tmp;
                tmp.fault_badge = current_cap_group->badge;
                tmp.fault_va = fault_va;
                set_one_msg(fault_pool->msg_buffer_kva, &tmp);
        }
        list_append(&pending_thread->node, &fault_pool->pending_threads);

        /* Notify the fault handler when buffer is updated */
        ret = signal_notific(fault_pool->notific);
        BUG_ON(ret != 0);

        /*
         * Give up the control flow here.
         * The thread will wake up when map finished.
         */
        thread_set_ts_blocking(current_thread);

        sched();
        /*
         * To avoid sys_user_fault_map get pending thread too early,
         *      or modify thread->state early than here.
         * Release lock here.
         */
        unlock(&fault_pool->lock);
        eret_to_thread(switch_context());
}
