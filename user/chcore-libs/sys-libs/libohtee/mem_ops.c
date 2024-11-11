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

#include <chcore/container/hashtable.h>
#include <chcore/memory.h>
#include <chcore/ipc.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore/syscall.h>
#include <chcore/opentrustee/syscall.h>

#include <string.h>
#include <pthread.h>

#include <mem_ops.h>
#include <ipclib.h>
#include <assert.h>

#define GTASK_PID    (7)
#define GTASK_TID    (0xa)
#define GTASK_TASKID (pid_to_taskid(GTASK_TID, GTASK_PID))

#define mem_info(fmt, ...) \
        printf("<%s:%d>: " fmt, __func__, __LINE__, ##__VA_ARGS__)

/* transfer @pmo to the cap group with @task_id */
static cap_t __task_map_ns(uint32_t task_id, cap_t pmo)
{
        ipc_msg_t *ipc_msg;
        struct proc_request *req;
        cap_t ret;

        ipc_msg = ipc_create_msg(procmgr_ipc_struct, sizeof(*req));
        req = (struct proc_request *)ipc_get_msg_data(ipc_msg);

        req->req = PROC_REQ_TRANSFER_PMO_CAP;
        req->transfer_pmo_cap.task_id = task_id;
        ipc_set_msg_cap(ipc_msg, 0, pmo);

        ret = ipc_call(procmgr_ipc_struct, ipc_msg);
        if (ret < 0) {
                goto out;
        }
        ret = req->transfer_pmo_cap.pmo;

out:
        ipc_destroy_msg(ipc_msg);
        return ret;
}

/*
 * Because pmo_deinit will remove all related mappings, revoking all pmo caps
 * here is enough.
 */
static int __task_unmap_ns(cap_t pmo)
{
        return usys_revoke_cap(pmo, true);
}

/*
 * Call procmgr to create shm only shared with @uuid.
 * @vaddr will be used to index this shm in the future when map_sharemem.
 */
static cap_t __alloc_sharemem(const struct tee_uuid *uuid, size_t size,
                              vaddr_t vaddr)
{
        ipc_msg_t *ipc_msg;
        struct proc_request *req;
        int ret;

        ipc_msg = ipc_create_msg(procmgr_ipc_struct, sizeof(*req));
        req = (struct proc_request *)ipc_get_msg_data(ipc_msg);

        req->req = PROC_REQ_TEE_ALLOC_SHM;
        req->tee_alloc_shm.vaddr = vaddr;
        memcpy(&req->tee_alloc_shm.uuid, uuid, sizeof(*uuid));
        req->tee_alloc_shm.size = size;

        ret = ipc_call(procmgr_ipc_struct, ipc_msg);

        ipc_destroy_msg(ipc_msg);
        return ret;
}

/*
 * Call procmgr to retrieve the pmo indexed by @task_id and @vaddr.
 */
static cap_t __get_sharemem(uint32_t task_id, vaddr_t vaddr)
{
        ipc_msg_t *ipc_msg;
        struct proc_request *req;
        int ret;
        cap_t pmo;

        ipc_msg = ipc_create_msg(procmgr_ipc_struct, sizeof(*req));
        req = (struct proc_request *)ipc_get_msg_data(ipc_msg);

        req->req = PROC_REQ_TEE_GET_SHM;
        req->tee_get_shm.pid = taskid_to_pid(task_id);
        req->tee_get_shm.vaddr = vaddr;

        ret = ipc_call(procmgr_ipc_struct, ipc_msg);
        if (ret < 0) {
                goto out;
        }

        pmo = ipc_get_msg_cap(ipc_msg, 0);
        if (pmo < 0) {
                ret = -EINVAL;
                goto out;
        }
        ret = pmo;

out:
        ipc_destroy_msg(ipc_msg);
        return ret;
}

/*
 * Call procmgr to free the pmo indexed by self task_id and @vaddr.
 */
static cap_t __free_sharemem(vaddr_t vaddr)
{
        ipc_msg_t *ipc_msg;
        struct proc_request *req;
        int ret;

        ipc_msg = ipc_create_msg(procmgr_ipc_struct, sizeof(*req));
        req = (struct proc_request *)ipc_get_msg_data(ipc_msg);

        req->req = PROC_REQ_TEE_FREE_SHM;
        req->tee_free_shm.vaddr = vaddr;

        ret = ipc_call(procmgr_ipc_struct, ipc_msg);

        ipc_destroy_msg(ipc_msg);
        return ret;
}

struct mem_entry {
        cap_t pmo;
        vaddr_t vaddr;
        uint32_t size;
        struct hlist_node pmo2ent_node;
        struct hlist_node vaddr2ent_node;
};

#define DEFAULT_HASHTABLE_SIZE 1024

static struct htable pmo2ent;
static struct htable vaddr2ent;
static pthread_mutex_t lock;

int mem_ops_init(void)
{
        pthread_mutex_init(&lock, NULL);
        init_htable(&pmo2ent, DEFAULT_HASHTABLE_SIZE);
        init_htable(&vaddr2ent, DEFAULT_HASHTABLE_SIZE);
        return 0;
}

static struct mem_entry *__get_entry_by_pmo(cap_t pmo)
{
        struct hlist_head *bucket = htable_get_bucket(&pmo2ent, pmo);
        struct mem_entry *entry;
        for_each_in_hlist (entry, pmo2ent_node, bucket) {
                if (entry->pmo == pmo) {
                        return entry;
                }
        }
        return NULL;
}

static struct mem_entry *__get_entry_by_vaddr(vaddr_t vaddr)
{
        struct hlist_head *bucket = htable_get_bucket(&vaddr2ent, vaddr);
        struct mem_entry *entry;
        for_each_in_hlist (entry, vaddr2ent_node, bucket) {
                if (entry->vaddr == vaddr) {
                        return entry;
                }
        }
        return NULL;
}

/* Create non-secure memory and mapping to caller. */
static int __map_ns_self(uint64_t phy_addr, uint32_t size, vaddr_t *out_vaddr)
{
        int ret;
        cap_t pmo;
        void *vaddr;
        struct mem_entry *entry;
        uint64_t phy_addr_begin, phy_addr_end;

        pthread_mutex_lock(&lock);

        phy_addr_end = ROUND_UP(phy_addr + size, PAGE_SIZE);
        phy_addr_begin = ROUND_DOWN(phy_addr, PAGE_SIZE);
        size = phy_addr_end - phy_addr_begin;

        entry = malloc(sizeof(*entry));
        if (entry == NULL) {
                ret = -ENOMEM;
                goto out;
        }

        pmo = usys_create_ns_pmo(phy_addr_begin, size);
        if (pmo < 0) {
                ret = pmo;
                goto out_free_entry;
        }

        vaddr = chcore_auto_map_pmo(pmo, size, VM_READ | VM_WRITE);
        if (vaddr == NULL) {
                ret = -errno;
                goto out_revoke_pmo;
        }
        vaddr = (void *)((uint64_t)vaddr + (phy_addr & (PAGE_SIZE - 1)));
        *out_vaddr = (vaddr_t)vaddr;

        entry->pmo = pmo;
        entry->vaddr = (vaddr_t)vaddr;
        entry->size = size;
        init_hlist_node(&entry->vaddr2ent_node);
        htable_add(&vaddr2ent, (vaddr_t)vaddr, &entry->vaddr2ent_node);

        entry = __get_entry_by_vaddr((vaddr_t)vaddr);

        pthread_mutex_unlock(&lock);
        return 0;

out_revoke_pmo:
        usys_revoke_cap(pmo, false);
out_free_entry:
        free(entry);
out:
        pthread_mutex_unlock(&lock);
        return ret;
}

/* remove non-secure memory and mapping indexed by @vaddr from caller. */
static int __unmap_ns_self(vaddr_t vaddr)
{
        int ret = 0;
        struct mem_entry *entry;

        pthread_mutex_lock(&lock);

        entry = __get_entry_by_vaddr(vaddr);
        if (entry == NULL) {
                ret = -ENOENT;
                goto out;
        }

        usys_unmap_pmo(0, entry->pmo, ROUND_DOWN(entry->vaddr, PAGE_SIZE));
        htable_del(&entry->vaddr2ent_node);
        usys_revoke_cap(entry->pmo, true);
        free(entry);

out:
        pthread_mutex_unlock(&lock);
        return ret;
}

int32_t task_map_ns_phy_mem(uint32_t task_id, uint64_t phy_addr, uint32_t size,
                            uint64_t *info)
{
        cap_t pmo;

        mem_info("%x %lx %x %p\n", task_id, phy_addr, size, info);

        if (task_id == 0) {
                task_id = GTASK_TASKID;
        }

        /*
         * When @task_id is caller itself, then the output(@info) is the
         * address of the mapping.
         * Otherwise, the output(@info) is the pmo cap in the cap group
         * with @task_id. This pmo cap will be sent to @task_id through
         * channel ipc.
         */
        if (task_id == get_self_taskid()) {
                int ret;
                vaddr_t vaddr;

                ret = __map_ns_self(phy_addr, size, &vaddr);
                if (ret == 0) {
                        *info = (uint64_t)vaddr;
                }
                return ret;
        } else {
                pmo = usys_create_ns_pmo(phy_addr, size);
                if (pmo < 0) {
                        return pmo;
                }
                pmo = __task_map_ns(task_id, pmo);
                if (pmo < 0) {
                        return pmo;
                } else {
                        *info = pmo;
                        return 0;
                }
        }
}

/*
 * Because each process manages its virtual address individually in ChCore,
 * each TA needs to map the received pmo(@info) using self_map_ns_phy_mem which
 * returns the @virt_addr of the mapping to caller.
 */
int32_t self_map_ns_phy_mem(uint64_t info, uint32_t size, uint64_t *virt_addr)
{
        int ret;
        void *addr;
        cap_t pmo = (cap_t)info;
        struct mem_entry *entry;

        pthread_mutex_lock(&lock);

        entry = __get_entry_by_pmo(pmo);
        if (entry != NULL) {
                ret = -EEXIST;
                goto out;
        }

        addr = chcore_auto_map_pmo(pmo, size, VM_READ | VM_WRITE);
        if (addr == NULL) {
                ret = -errno;
                goto out;
        }

        entry = malloc(sizeof(*entry));
        if (entry == NULL) {
                ret = -ENOMEM;
                goto out_unmap;
        }
        entry->pmo = pmo;
        entry->vaddr = (vaddr_t)addr;
        entry->size = size;
        init_hlist_node(&entry->pmo2ent_node);
        init_hlist_node(&entry->vaddr2ent_node);
        htable_add(&pmo2ent, pmo, &entry->pmo2ent_node);
        htable_add(
                &vaddr2ent, (u32)(unsigned long)addr, &entry->vaddr2ent_node);

        *virt_addr = (uint64_t)addr;
        pthread_mutex_unlock(&lock);
        return 0;

out_unmap:
        chcore_auto_unmap_pmo(pmo, (unsigned long)addr, size);
out:
        pthread_mutex_unlock(&lock);
        return ret;
}

int32_t task_unmap(uint32_t task_id, uint64_t info, uint32_t size)
{
        int ret;

        if (task_id == 0) {
                task_id = GTASK_TASKID;
        }

        if (task_id == get_self_taskid()) {
                ret = __unmap_ns_self(info);
        } else {
                /* revoke all caps and all mappings of the pmo(@info) */
                ret = __task_unmap_ns((cap_t)info);
        }
        return ret;
}

/* 
 * Free virtual address and local data structure.
 */
int32_t self_unmap(uint64_t info)
{
        struct mem_entry *entry;
        cap_t pmo = (cap_t)info;
        int ret = 0;

        pthread_mutex_lock(&lock);

        entry = __get_entry_by_pmo(pmo);
        if (entry == NULL) {
                ret = -ENOENT;
                goto out;
        }

        chcore_free_vaddr(entry->vaddr, entry->size);

        htable_del(&entry->pmo2ent_node);
        htable_del(&entry->vaddr2ent_node);
        free(entry);
        /* pmo has been already revoked by gtask through task_unmap */

out:
        pthread_mutex_unlock(&lock);
        return ret;
}

/* Allocate shared memory. Only process with @uuid can share this memory. */
void *alloc_sharemem_aux(const struct tee_uuid *uuid, uint32_t size)
{
        vaddr_t vaddr;
        int ret;
        struct mem_entry *entry;
        cap_t pmo;

        size = ROUND_UP(size, PAGE_SIZE);
        pthread_mutex_lock(&lock);

        vaddr = (vaddr_t)chcore_alloc_vaddr(size);
        if (vaddr == 0) {
                goto out_fail;
        }

        entry = malloc(sizeof(*entry));
        if (entry == NULL) {
                goto out_fail_free_vaddr;
        }

        pmo = __alloc_sharemem(uuid, size, vaddr);
        if (pmo < 0) {
                goto out_free_entry;
        }

        ret = usys_map_pmo(0, pmo, vaddr, VM_READ | VM_WRITE);
        if (ret != 0) {
                goto out_free_sharemem;
        }

        entry->pmo = pmo;
        entry->vaddr = (vaddr_t)vaddr;
        entry->size = size;
        init_hlist_node(&entry->vaddr2ent_node);
        htable_add(&vaddr2ent, (u32)(vaddr_t)vaddr, &entry->vaddr2ent_node);

        pthread_mutex_unlock(&lock);
        return (void *)vaddr;

out_free_sharemem:
        __free_sharemem((vaddr_t)vaddr);
out_free_entry:
        free(entry);
out_fail_free_vaddr:
        chcore_free_vaddr(vaddr, size);
out_fail:
        pthread_mutex_unlock(&lock);
        return NULL;
}

uint32_t free_sharemem(void *addr, uint32_t size)
{
        int ret;
        struct mem_entry *entry;

        pthread_mutex_lock(&lock);

        entry = __get_entry_by_vaddr((vaddr_t)addr);
        if (entry == NULL) {
                ret = -ENOENT;
                goto out;
        }

        ret = __free_sharemem((vaddr_t)addr);
        if (ret < 0) {
                goto out;
        }

        chcore_auto_unmap_pmo(entry->pmo, entry->vaddr, entry->size);
        ret = usys_revoke_cap(entry->pmo, false);
        htable_del(&entry->vaddr2ent_node);
        chcore_free_vaddr(entry->vaddr, entry->size);
        free(entry);

out:
        pthread_mutex_unlock(&lock);
        return ret;
}

/*
 * Map shm in @src_task with @vaddr and @size to caller's vmspace. The address
 * of the mapping will be stored in @vaddr_out.
 */
int32_t map_sharemem(uint32_t src_task, uint64_t vaddr, uint64_t size,
                     uint64_t *vaddr_out)
{
        int ret;
        cap_t pmo;
        void *out_vaddr;
        struct mem_entry *entry;

        if (src_task == 0) {
                src_task = GTASK_TASKID;
        }

        pthread_mutex_lock(&lock);

        pmo = __get_sharemem(src_task, vaddr);
        if (pmo < 0) {
                ret = pmo;
                goto out;
        }

        entry = malloc(sizeof(*entry));
        if (entry == NULL) {
                ret = -ENOMEM;
                goto out_revoke_pmo;
        }

        out_vaddr = chcore_auto_map_pmo(pmo, size, VM_READ | VM_WRITE);
        if (out_vaddr == NULL) {
                ret = -errno;
                goto out_free_entry;
        }
        *vaddr_out = (uint64_t)out_vaddr;

        entry->pmo = pmo;
        entry->vaddr = (vaddr_t)out_vaddr;
        entry->size = size;
        init_hlist_node(&entry->vaddr2ent_node);
        init_hlist_node(&entry->pmo2ent_node);
        htable_add(&vaddr2ent, (u32)(vaddr_t)vaddr, &entry->vaddr2ent_node);

        pthread_mutex_unlock(&lock);
        return 0;

out_free_entry:
        free(entry);
out_revoke_pmo:
        usys_revoke_cap(pmo, false);
out:
        pthread_mutex_unlock(&lock);
        return ret;
}

uint64_t virt_to_phys(uintptr_t vaddr)
{
        unsigned long paddr;
        int ret;

        ret = usys_get_phys_addr((void *)vaddr, &paddr);

        if (ret) {
                return 0;
        } else {
                return paddr;
        }
}

int32_t dump_mem_info(struct stat_mem_info *info, int print_history)
{
        printf("%s not implemented!\n", __func__);
        return 0;
}