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

#ifndef FS_VNODE_H
#define FS_VNODE_H

#include <chcore/type.h>
#include <assert.h>
#include <sys/types.h>
#include <chcore/container/list.h>
#include <chcore/container/hashtable.h>
#include <chcore/container/rbtree.h>

#include "fs_page_cache.h"
#include "fs_wrapper_defs.h"

#define MAX_FILE_PAGES       512
#define MAX_SERVER_ENTRY_NUM 1024

enum fs_vnode_type { FS_NODE_RESERVED = 0, FS_NODE_REG, FS_NODE_DIR };

/*
 * per-inode
 */
#define PC_HASH_SIZE 512
struct fs_vnode {
        ino_t vnode_id; /* identifier */
        struct rb_node node; /* rbtree node */

        enum fs_vnode_type type; /* regular or directory */
        int refcnt; /* reference count */
        off_t size; /* file size or directory entry number */
        struct page_cache_entity_of_inode *page_cache;
        cap_t pmo_cap; /* fmap fault is handled by this */
        void *private;

        pthread_rwlock_t rwlock; /* vnode rwlock */
};

/*
 * per-fd
 */
struct server_entry {
        /* `flags` and `offset` is assigned to each fd */
        int flags;
        off_t offset;
        int refcnt;
        /*
         * Different FS may use different struct to store path,
         * normally `char*`
         */
        void *path;

        /* Entry lock */
        pthread_mutex_t lock;

        /* Each vnode is binding with a disk inode */
        struct fs_vnode *vnode;
};

extern struct server_entry *server_entrys[MAX_SERVER_ENTRY_NUM];

extern bool using_page_cache;

extern void free_entry(int entry_idx);
extern int alloc_entry(void);
extern void assign_entry(struct server_entry *e, u64 f, off_t o, int t, void *p,
                         struct fs_vnode *n);

/*
 * fs_vnode pool
 * key: ino_t vnode_id
 * value: struct fs_vnode *vnode
 */
extern struct rb_root *fs_vnode_list;

extern void fs_vnode_init(void);
extern struct fs_vnode *alloc_fs_vnode(ino_t id, enum fs_vnode_type type,
                                       off_t size, void *private);
extern void push_fs_vnode(struct fs_vnode *n);
extern void pop_free_fs_vnode(struct fs_vnode *n);
extern struct fs_vnode *get_fs_vnode_by_id(ino_t vnode_id);

int inc_ref_fs_vnode(void *);
int dec_ref_fs_vnode(void *);

#endif /* FS_VNODE_H */