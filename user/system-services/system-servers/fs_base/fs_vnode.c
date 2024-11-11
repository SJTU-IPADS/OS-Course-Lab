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

#include <chcore-internal/fs_debug.h>
#include <chcore/syscall.h>
#include <stdio.h>
#include <stdlib.h>

#include "fs_vnode.h"
#include "fs_page_cache.h"

static int comp_vnode_key(const void *key, const struct rb_node *node)
{
        struct fs_vnode *vnode = rb_entry(node, struct fs_vnode, node);
        ino_t vnode_id = *(ino_t *)key;

        if (vnode_id < vnode->vnode_id)
                return -1;
        else if (vnode_id > vnode->vnode_id)
                return 1;
        else
                return 0;
}

static bool less_vnode(const struct rb_node *lhs, const struct rb_node *rhs)
{
        struct fs_vnode *l = rb_entry(lhs, struct fs_vnode, node);
        struct fs_vnode *r = rb_entry(rhs, struct fs_vnode, node);

        return l->vnode_id < r->vnode_id;
}

void free_entry(int entry_idx)
{
        free(server_entrys[entry_idx]->path);
        free(server_entrys[entry_idx]);
        server_entrys[entry_idx] = NULL;
}

int alloc_entry(void)
{
        int i;

        for (i = 0; i < MAX_SERVER_ENTRY_NUM; i++) {
                if (server_entrys[i] == NULL) {
                        server_entrys[i] = (struct server_entry *)malloc(
                                sizeof(struct server_entry));
                        if (server_entrys[i] == NULL)
                                return -1;
                        pthread_mutex_init(&server_entrys[i]->lock, NULL);
                        fs_debug_trace_fswrapper("entry_id=%d\n", i);
                        return i;
                }
        }
        return -1;
}

void assign_entry(struct server_entry *e, u64 f, off_t o, int t, void *p,
                  struct fs_vnode *n)
{
        fs_debug_trace_fswrapper(
                "flags=0x%lo, offset=0x%ld, path=%s, vnode_id=%ld\n",
                f,
                o,
                (char *)p,
                n->vnode_id);
        e->flags = f;
        e->offset = o;
        e->path = p;
        e->vnode = n;
        e->refcnt = t;
}

void fs_vnode_init(void)
{
        fs_vnode_list = malloc(sizeof(*fs_vnode_list));
        if (fs_vnode_list == NULL) {
                printf("[fs_base] no enough memory to initialize, exiting...\n");
                exit(-1);
        }
        init_rb_root(fs_vnode_list);
}

struct fs_vnode *alloc_fs_vnode(ino_t id, enum fs_vnode_type type, off_t size,
                                void *private)
{
        struct fs_vnode *ret = (struct fs_vnode *)malloc(sizeof(*ret));
        if (ret == NULL) {
                return NULL;
        }

        /* Filling Initial State */
        ret->vnode_id = id;
        ret->type = type;
        ret->size = size;
        ret->private = private;
        fs_debug_trace_fswrapper(
                "id=%ld, type=%d, size=%ld(0x%lx)\n", id, type, size, size);

        /* Ref Count start as 1 */
        ret->refcnt = 1;

        ret->pmo_cap = -1;

        /* Create a page cache entity for vnode */
        if (using_page_cache)
                ret->page_cache =
                        new_page_cache_entity_of_inode(ret->vnode_id, ret);
        pthread_rwlock_init(&ret->rwlock, NULL);

        return ret;
}

void push_fs_vnode(struct fs_vnode *n)
{
        rb_insert(fs_vnode_list, &n->node, less_vnode);
}

void pop_free_fs_vnode(struct fs_vnode *n)
{
        rb_erase(fs_vnode_list, &n->node);
        if (n->pmo_cap > 0) {
                usys_revoke_cap(n->pmo_cap, false);
        }
        free(n);
}

struct fs_vnode *get_fs_vnode_by_id(ino_t vnode_id)
{
        struct rb_node *node =
                rb_search(fs_vnode_list, &vnode_id, comp_vnode_key);
        if (node == NULL)
                return NULL;
        return rb_entry(node, struct fs_vnode, node);
}

/* refcnt for vnode */
int inc_ref_fs_vnode(void *n)
{
        ((struct fs_vnode *)n)->refcnt++;
        return 0;
}

int dec_ref_fs_vnode(void *node)
{
        int ret;
        struct fs_vnode *n = (struct fs_vnode *)node;

        n->refcnt--;
        assert(n->refcnt >= 0);

        if (n->refcnt == 0) {
                ret = server_ops.close(
                        n->private, (n->type == FS_NODE_DIR), true);
                if (ret) {
                        printf("Warning: close failed when deref vnode: %d\n",
                               ret);
                        return ret;
                }

                pop_free_fs_vnode(n);
        }

        return 0;
}