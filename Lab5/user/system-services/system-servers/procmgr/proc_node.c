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
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <chcore/idman.h>
#include <chcore-internal/fs_defs.h>
#include <chcore/container/hashtable.h>
#include <chcore/proc.h>

#include "proc_node.h"
#include "procmgr_dbg.h"
#include "srvmgr.h"

/* For synchronization */
static pthread_mutex_t proc_nodes_lock;

/* For allocating pid to proc_node */
static struct id_manager pid_mgr;
static const int PID_MAX = 1024 * 1024;

/* Map: client_badge -> proc_node */
/*
 * We use client_badge as the index of procnode.
 */
static struct htable badge2proc;
static struct htable pid2proc;

/* Max number of pcid on x86_64 is different from that (ASID) on aarch64. */
static struct id_manager pcid_mgr;
#if defined(CHCORE_ARCH_X86_64)
static const int PCID_MAX = 1 << 12;
#elif defined(CHCORE_ARCH_AARCH64)
static const int PCID_MAX = 1 << 16;
#elif defined(CHCORE_ARCH_RISCV64)
static const int PCID_MAX = 1 << 16;
#elif defined(CHCORE_ARCH_SPARC)
static const int PCID_MAX = 1 << 16;
#else
#error "Unsupported architecture"
#endif

#define HASH_TABLE_SIZE 509
/*
 * PCID in range [0,10) is reserved for boot page table, root process,
 * fsm, fs, lwip, procmgr and future servers (pcid 0 is not used).
 * Thus the user apps' pcid starts at 10. (10 is used by init process)
 */
static const int MAX_RESERVED_PCID = 10;

/* Only for handle_init */
static struct proc_node *proc_init;

static inline badge_t generate_server_badge(void)
{
        static badge_t cur_server_badge = MIN_FREE_SERVER_BADGE;
        BUG_ON(++cur_server_badge >= MIN_FREE_DRIVER_BADGE);
        return cur_server_badge;
}

static inline badge_t generate_driver_badge(void)
{
        static badge_t cur_driver_badge = MIN_FREE_DRIVER_BADGE;
        BUG_ON(++cur_driver_badge >= MIN_FREE_APP_BADGE);
        return cur_driver_badge;
}

static inline badge_t generate_app_badge(void)
{
        static badge_t cur_app_badge = MIN_FREE_APP_BADGE;
        return cur_app_badge++;
}

static struct proc_node *__new_proc_node(struct proc_node *parent, char *name)
{
        struct proc_node *proc = malloc(sizeof(*proc));
        assert(proc);

        /* Alloc pid */
        pthread_mutex_lock(&proc_nodes_lock);
        proc->pid = alloc_id(&pid_mgr);
        BUG_ON(proc->pid == -EINVAL);
        pthread_mutex_unlock(&proc_nodes_lock);

        proc->name = name;
        proc->parent = parent;
        proc->state = PROC_STATE_INIT;
        pthread_mutex_init(&proc->lock, NULL);
        if (proc->parent) {
                /* Add this proc to parent's child list. */
                pthread_mutex_lock(&parent->lock);
                list_add(&proc->node, &proc->parent->children);
                pthread_mutex_unlock(&parent->lock);
        }
        init_list_head(&proc->children);
        init_hlist_node(&proc->hash_node);
        init_hlist_node(&proc->pid_hash_node);

        return proc;
}

void init_proc_node_mgr(void)
{
        init_id_manager(&pid_mgr, PID_MAX, DEFAULT_INIT_ID);

        pthread_mutex_init(&proc_nodes_lock, NULL);
        pthread_mutex_init(&recycle_lock, NULL);
        /* Reserve the pcid for root process and servers. */
        init_id_manager(&pcid_mgr, PCID_MAX, MAX_RESERVED_PCID);

        init_htable(&badge2proc, HASH_TABLE_SIZE);
        init_htable(&pid2proc, HASH_TABLE_SIZE);
}

/*
 * The name here should be a newly allocated memory that can be directly stored
 * (and sometime later freed) in the proc_node.
 */
struct proc_node *new_proc_node(struct proc_node *parent, char *name,
                                int proc_type)
{
        struct proc_node *proc = __new_proc_node(parent, name);

        pthread_mutex_lock(&proc_nodes_lock);

        /* Alloc pcid */
        if (strcmp(name, "procmgr") == 0 && proc_type == SYSTEM_SERVER) {
                proc->pcid = PROCMGR_PCID;
                proc->badge = PROCMGR_BADGE;
        } else if (strcmp(name, "fsm") == 0 && proc_type == SYSTEM_SERVER) {
                proc->pcid = FSM_PCID;
                proc->badge = FSM_BADGE;
        } else if (strcmp(name, "lwip") == 0 && proc_type == SYSTEM_SERVER) {
                proc->pcid = LWIP_PCID;
                proc->badge = LWIP_BADGE;
        } else if (proc_type == SYSTEM_SERVER) {
                proc->pcid = alloc_id(&pcid_mgr);
                proc->badge = generate_server_badge();
        } else if (proc_type == SYSTEM_DRIVER) {
                proc->pcid = alloc_id(&pcid_mgr);
                proc->badge = generate_driver_badge();
        } else if (proc_type == COMMON_APP || proc_type == TRACED_APP) {
                proc->pcid = alloc_id(&pcid_mgr);
                proc->badge = generate_app_badge();
        } else {
                warn("new proc failed, proc type error %d\n", proc_type);
                return NULL;
        }
        BUG_ON(proc->pcid == -EINVAL);

        /* Generate badge and add to htable */
        htable_add(&badge2proc, proc->badge, &proc->hash_node);
        htable_add(&pid2proc, proc->pid, &proc->pid_hash_node);

        pthread_mutex_unlock(&proc_nodes_lock);
        debug("alloc pcid = %d\n", proc->pcid);
        return proc;
}

void free_proc_node_resource(struct proc_node *proc)
{
        int pid;
        int pcid;

        pthread_mutex_lock(&proc_nodes_lock);

        pid = proc->pid;
        pcid = (int)proc->pcid;

        /* Just delete the node in the hash table. Free proc later. */
        htable_del(&proc->hash_node);
        htable_del(&proc->pid_hash_node);

        free_id(&pid_mgr, pid);
        free_id(&pcid_mgr, pcid);
        debug("free pcid = %d\n", pcid);

        if (proc->name)
                free(proc->name);
        pthread_mutex_unlock(&proc_nodes_lock);
}

/* Free the resource allocated in new_proc_node in a reverse order */
void del_proc_node(struct proc_node *proc)
{
        struct proc_node *child;
        struct proc_node *tmp;

        BUG_ON(proc->state != PROC_STATE_EXIT);

        /*
         * Step 1. Set the child proc node as orphan, delete the child list of
         * the proc node and free all exited child node.
         */
        for_each_in_list_safe (child, tmp, node, &proc->children) {
                child->parent = NULL;
                /*
                 * NOTE: If we need keep the relationship between the child of
                 * the proc_node, we need append the child node to a new
                 * process(such as init).
                 */
                /* Recycle exited child proc node. */
                if (child->state == PROC_STATE_EXIT) {
                        free_proc_node_resource(child);
                        free(child);
                }
        }

        /*
         * Step2. If the proc is orphan, free the proc node.
         */
        if (!proc->parent) {
                free_proc_node_resource(proc);
                free(proc);
        }
}

struct proc_node *get_proc_node(badge_t client_badge)
{
        struct proc_node *proc;
        struct hlist_head *buckets;

        pthread_mutex_lock(&proc_nodes_lock);
        buckets = htable_get_bucket(&badge2proc, client_badge);

        for_each_in_hlist (proc, hash_node, buckets) {
                if (client_badge == proc->badge) {
                        goto out;
                }
        }
        pthread_mutex_unlock(&proc_nodes_lock);
        return NULL;
out:
        debug("Find badge = 0x%x, get proc = %p\n", client_badge, proc);
        pthread_mutex_unlock(&proc_nodes_lock);

        return proc;
}

struct proc_node *get_proc_node_by_pid(int pid)
{
        struct proc_node *proc;
        struct hlist_head *buckets;

        pthread_mutex_lock(&proc_nodes_lock);
        buckets = htable_get_bucket(&pid2proc, pid);

        for_each_in_hlist(proc, pid_hash_node, buckets) {
                if (pid == proc->pid) {
                        goto out;
                }
        }
        pthread_mutex_unlock(&proc_nodes_lock);
        return NULL;
out:
        debug("Find pid = %d, get proc = %p\n", pid, proc);
        pthread_mutex_unlock(&proc_nodes_lock);

        return proc;
}

void init_root_proc_node(void)
{
        /* Init the init node. */
        proc_init = new_proc_node(NULL, strdup("procmgr"), SYSTEM_SERVER);
        htable_add(&badge2proc, proc_init->badge, &proc_init->hash_node);
        htable_add(&pid2proc, proc_init->pid, &proc_init->pid_hash_node);

        __sync_synchronize();
}
