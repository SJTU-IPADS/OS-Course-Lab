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

#ifndef PROC_NODE_H
#define PROC_NODE_H

#ifdef CHCORE_OPENTRUSTEE
#include <spawn_ext.h>
#endif /* CHCORE_OPENTRUSTEE */
#include <pthread.h>
#include <chcore/container/list.h>
#include <chcore/ipc.h>
#include <chcore-internal/procmgr_defs.h>

#define INIT_BADGE 1

enum proc_state {
        PROC_STATE_INIT = 1,
        PROC_STATE_RUNNING,
        PROC_STATE_EXIT,
        PROC_STATE_MAX
};

struct proc_node {
        char *name; /* The name of the process. */
        /* The capability of the process owned in procmgr. */
        cap_t proc_cap;
        /* The capability of the process's main thread (MT) owned in procmgr. */
        cap_t proc_mt_cap;
        pid_t pid;
        u64 pcid; /* Used for initializing pgtbl */
        badge_t badge; /* Used for recognizing a process. */
        size_t stack_size;

        pthread_cond_t wait_cv;

        volatile enum proc_state state;
        int exitstatus;

        /* Connecters */
        struct proc_node *parent;
        /* A lock: used to coordinate the access to child procs list */
        struct list_head children; /* A list of child procs. */
        struct list_head node; /* The node in the parent's child list. */

        struct hlist_node hash_node; /* node in badge2proc hash table */
        struct hlist_node pid_hash_node; /* node in pid2proc hash table */

#ifdef CHCORE_OPENTRUSTEE
        spawn_uuid_t puuid;
#endif /* CHCORE_OPENTRUSTEE */
};

void init_proc_node_mgr(void);

int new_proc_node(struct proc_node *parent, char *name,
                  int proc_type, struct proc_node **out_proc);
/*
 * Undo new_proc_node. Remove proc from global data structure and put current
 * reference.
 */
void del_proc_node(struct proc_node *proc);

/*
 * Remove proc from global data structure. Used in recycle
 */
void proc_node_exit(struct proc_node *proc);

void signal_waiters(struct proc_node *proc);
pid_t wait_reap_child(struct proc_node *proc, pid_t pid, int *status);

/*
 * These **get** and **put** operations need to be used in pairs.
 * The **get** operation returns the reference with reference count increased.
 * The **put** operation decreases the reference count.
 */
struct proc_node *get_proc_node(badge_t client_badge);
struct proc_node *get_proc_node_by_pid(pid_t pid);
struct proc_node *get_child(struct proc_node *proc_node, pid_t pid);
void put_proc_node(struct proc_node *proc);

void init_root_proc_node(void);
void for_all_proc_do(void (*func)(struct proc_node *proc));

#endif /* PROC_NODE_H */