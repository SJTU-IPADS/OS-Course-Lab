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
#include "librefcnt.h"

/*
 * References from the global hash table:
 *   1. Calling proc_node_exit in the recycle function will remove the
 *      proc_node from the hash table and release the reference.
 * References from the process tree:
 *   1. Calling proc_node_exit in the recycle function will release the
 *      parent-child references between the proc_node and its child processes,
 *      thereby marking the child processes as orphans.
 *   2. After waiting for a child process, the parent-child references will be
 *      released.
 *
 * In conclusion, after all temporary references have been released, the
 * reference count will reach 0 if the process has been reaped by the parent
 * process or if the parent process has already exited. This will trigger the
 * recycling of the proc_node.
 */

/* For synchronization */
static pthread_mutex_t proc_nodes_lock;
static pthread_mutex_t proc_id_lock;
static pthread_mutex_t proc_tree_lock;

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

static int init_proc_node(struct proc_node *proc, char *name, int proc_type)
{
        int ret = 0;

        proc->parent = NULL;
        proc->name = name;
        proc->state = PROC_STATE_INIT;

        pthread_cond_init(&proc->wait_cv, NULL);

        init_list_head(&proc->children);
        init_hlist_node(&proc->hash_node);
        init_hlist_node(&proc->pid_hash_node);

        pthread_mutex_lock(&proc_id_lock);

#ifndef CHCORE_OPENTRUSTEE
        /* Alloc pid */
        proc->pid = alloc_id(&pid_mgr);
        if (proc->pid == -EINVAL) {
                ret = -EINVAL;
                goto out_unlock;
        }
#endif /* CHCORE_OPENTRUSTEE */

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
                proc->pcid = -EINVAL;
        }

        if (proc->pcid == -EINVAL) {
                ret = -EINVAL;
                goto out_free_pid;
        }

#ifdef CHCORE_OPENTRUSTEE
        /* Let pid = badge */
        proc->pid = proc->badge;
#endif /* CHCORE_OPENTRUSTEE */

        pthread_mutex_unlock(&proc_id_lock);
        return 0;

out_free_pid:
#ifndef CHCORE_OPENTRUSTEE
        free_id(&pid_mgr, proc->pid);
#endif /* CHCORE_OPENTRUSTEE */
out_unlock:
        pthread_mutex_unlock(&proc_id_lock);
        return ret;
}

static void deinit_proc_node(struct proc_node *proc_node)
{
        int pid;
        int pcid;
        debug("%s %d\n", __func__, __LINE__);

        pthread_mutex_lock(&proc_id_lock);

        pid = proc_node->pid;
        pcid = (int)proc_node->pcid;

#ifndef CHCORE_OPENTRUSTEE
        free_id(&pid_mgr, pid);
#endif /* CHCORE_OPENTRUSTEE */
        free_id(&pcid_mgr, pcid);
        debug("free pcid = %d\n", pcid);

        pthread_mutex_unlock(&proc_id_lock);

        if (proc_node->name)
                free(proc_node->name);
}

void proc_node_deinit(void *ptr)
{
        deinit_proc_node((struct proc_node *)ptr);
}

static void __add_proc_node_to_parent_locked(struct proc_node *proc,
                                             struct proc_node *parent)
{
        proc->parent = obj_get(parent);
        obj_get(proc);
        list_add(&proc->node, &parent->children);
}

static void __del_proc_node_from_parent_locked(struct proc_node *proc)
{
        obj_put(proc->parent);
        proc->parent = NULL;
        list_del(&proc->node);
        obj_put(proc);
}

static void __add_proc_node_to_global_table(struct proc_node *proc)
{
        pthread_mutex_lock(&proc_nodes_lock);
        obj_get(proc);
        htable_add(&badge2proc, proc->badge, &proc->hash_node);
        htable_add(&pid2proc, proc->pid, &proc->pid_hash_node);
        pthread_mutex_unlock(&proc_nodes_lock);
}

static void __del_proc_node_from_global_table(struct proc_node *proc)
{
        pthread_mutex_lock(&proc_nodes_lock);
        htable_del(&proc->hash_node);
        htable_del(&proc->pid_hash_node);
        obj_put(proc);
        pthread_mutex_unlock(&proc_nodes_lock);
}

void init_proc_node_mgr(void)
{
        init_id_manager(&pid_mgr, PID_MAX, DEFAULT_INIT_ID);

        pthread_mutex_init(&proc_nodes_lock, NULL);
        pthread_mutex_init(&proc_id_lock, NULL);
        pthread_mutex_init(&proc_tree_lock, NULL);
        /* Reserve the pcid for root process and servers. */
        init_id_manager(&pcid_mgr, PCID_MAX, MAX_RESERVED_PCID);

        init_htable(&badge2proc, HASH_TABLE_SIZE);
        init_htable(&pid2proc, HASH_TABLE_SIZE);
}

/*
 * The name here should be a newly allocated memory that can be directly stored
 * (and sometime later freed) in the proc_node.
 */
int new_proc_node(struct proc_node *parent, char *name, int proc_type,
                  struct proc_node **out_proc)
{
        struct proc_node *proc;
        int ret;

        proc = obj_alloc(sizeof(*proc), proc_node_deinit);
        if (proc == NULL) {
                ret = -ENOMEM;
                goto out_fail;
        }

        ret = init_proc_node(proc, name, proc_type);
        if (ret != 0) {
                goto out_put_proc;
        }

        if (parent) {
                pthread_mutex_lock(&proc_tree_lock);
                __add_proc_node_to_parent_locked(proc, parent);
                pthread_mutex_unlock(&proc_tree_lock);
        }

        __add_proc_node_to_global_table(proc);

        debug("alloc pcid = %d\n", proc->pcid);

        if (out_proc) {
                *out_proc = proc;
        } else {
                obj_put(proc);
        }
        return 0;

out_put_proc:
        obj_put(proc);
out_fail:
        return ret;
}

void del_proc_node(struct proc_node *proc)
{
        __del_proc_node_from_global_table(proc);
        if (proc->parent) {
                pthread_mutex_lock(&proc_tree_lock);
                __del_proc_node_from_parent_locked(proc);
                pthread_mutex_unlock(&proc_tree_lock);
        }

        obj_put(proc);
}

void signal_waiters(struct proc_node *proc)
{
        pthread_mutex_lock(&proc_tree_lock);

        proc->state = PROC_STATE_EXIT;

        if (proc->parent) {
                pthread_cond_broadcast(&proc->parent->wait_cv);
        }

        pthread_mutex_unlock(&proc_tree_lock);
}

void proc_node_exit(struct proc_node *proc)
{
        struct proc_node *child;
        struct proc_node *tmp;

        BUG_ON(proc->state != PROC_STATE_EXIT);

        /* Set the child proc node as orphan */
        pthread_mutex_lock(&proc_tree_lock);
        for_each_in_list_safe (child, tmp, node, &proc->children) {
                __del_proc_node_from_parent_locked(child);
        }
        pthread_mutex_unlock(&proc_tree_lock);

        __del_proc_node_from_global_table(proc);
}

/*
 * In these **get** functions, since the global data structure holds references
 * to the proc_nodes in the data structure and the data structure lock is
 * LOCKED, these proc_nodes will not be released by others in the critical
 * section. Therefore, memory safety can be guaranteed.
 */
struct proc_node *get_proc_node(badge_t client_badge)
{
        struct proc_node *proc;
        struct hlist_head *buckets;

        pthread_mutex_lock(&proc_nodes_lock);
        buckets = htable_get_bucket(&badge2proc, client_badge);

        for_each_in_hlist (proc, hash_node, buckets) {
                if (client_badge == proc->badge) {
                        obj_get(proc);
                        goto found;
                }
        }
        pthread_mutex_unlock(&proc_nodes_lock);
        return NULL;

found:
        debug("Find badge = 0x%x, get proc = %p\n", client_badge, proc);
        pthread_mutex_unlock(&proc_nodes_lock);

        return proc;
}

struct proc_node *get_proc_node_by_pid(pid_t pid)
{
        struct proc_node *proc;
        struct hlist_head *buckets;

        pthread_mutex_lock(&proc_nodes_lock);
        buckets = htable_get_bucket(&pid2proc, pid);

        for_each_in_hlist (proc, pid_hash_node, buckets) {
                if (pid == proc->pid) {
                        obj_get(proc);
                        goto found;
                }
        }
        pthread_mutex_unlock(&proc_nodes_lock);
        return NULL;

found:
        debug("Find pid = %d, get proc = %p\n", pid, proc);
        pthread_mutex_unlock(&proc_nodes_lock);

        return proc;
}

static struct proc_node *__get_child_locked(struct proc_node *proc_node,
                                            pid_t pid)
{
        struct proc_node *child;
        struct proc_node *ret = NULL;

        for_each_in_list (child, struct proc_node, node, &proc_node->children) {
                if (pid == -1 || child->pid == pid) {
                        ret = obj_get(child);
                        break;
                }
        }

        return ret;
}

struct proc_node *get_child(struct proc_node *proc_node, pid_t pid)
{
        struct proc_node *ret = NULL;

        pthread_mutex_lock(&proc_tree_lock);
        ret = __get_child_locked(proc_node, pid);
        pthread_mutex_unlock(&proc_tree_lock);
        return ret;
}

/*
 * If there is no child corresponding to the given pid, return -ECHILD.
 * If there is an exited child corresponding to the given PID, release the
 * resources associated with the child and return the pid.
 * Otherwise, wait for the exit event of all children. Once awakened, return 0.
 */
pid_t wait_reap_child(struct proc_node *proc, pid_t pid, int *status)
{
        struct proc_node *child;
        struct proc_node *chosen = NULL;
        pid_t ret = 0;

        pthread_mutex_lock(&proc_tree_lock);

        /*
         * Check if the child process corresponding to the given pid exists.
         * Return -ECHILD if not.
         */
        child = __get_child_locked(proc, pid);
        if (child == NULL) {
                ret = -ECHILD;
                goto out_unlock;
        }
        obj_put(child);

        /*
         * Check if there are any exited child processes among the children
         * corresponding to the given pid.
         * Wait for them if not.
         */
        for_each_in_list (child, struct proc_node, node, &proc->children) {
                if ((pid == -1 || child->pid == pid)
                    && child->state == PROC_STATE_EXIT) {
                        chosen = obj_get(child);
                        break;
                }
        }

        if (chosen) {
                *status = chosen->exitstatus;
                ret = chosen->pid;
                BUG_ON(ret == 0);
                __del_proc_node_from_parent_locked(chosen);
                obj_put(chosen);
        } else {
                pthread_cond_wait(&proc->wait_cv, &proc_tree_lock);
        }

out_unlock:
        pthread_mutex_unlock(&proc_tree_lock);
        return ret;
}

void put_proc_node(struct proc_node *proc)
{
        obj_put(proc);
}

void init_root_proc_node(void)
{
        int ret;
        /* Init the init node. */
        ret = new_proc_node(NULL, strdup("procmgr"), SYSTEM_SERVER, &proc_init);
        BUG_ON(ret);

        __sync_synchronize();
}

void for_all_proc_do(void (*func)(struct proc_node *proc))
{
        int i;
        struct proc_node *proc;

        pthread_mutex_lock(&proc_nodes_lock);

        for_each_in_htable (proc, i, hash_node, &badge2proc) {
                func(proc);
        }

        pthread_mutex_unlock(&proc_nodes_lock);
}
