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

void assign_entry(struct server_entry *entry, u64 flags, off_t offset, int refcnt, void *path,
                  struct fs_vnode *vnode)
{
        fs_debug_trace_fswrapper(
                "flags=0x%lo, offset=0x%ld, path=%s, vnode_id=%ld\n",
                flags,
                offset,
                (char *)path,
                vnode->vnode_id);
        entry->flags = flags;
        entry->offset = offset;
        entry->path = path;
        entry->vnode = vnode;
        entry->refcnt = refcnt;
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

/* alloc fs_vnode & initalize its states */
struct fs_vnode *alloc_fs_vnode(ino_t id, enum fs_vnode_type type, off_t size,
                                void *private)
{
        /* Lab 5 TODO Begin */

        return NULL;

        /* Lab 5 TODO End */
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
        /* Lab 5 TODO Begin */
        
        return NULL;

        /* Lab 5 TODO End */
}

/* increase refcnt for vnode */
int inc_ref_fs_vnode(void *n)
{
        /* Lab 5 TODO Begin */


        /* Lab 5 TODO End */
        return 0;
}

/* decrease vnode ref count and close file when refcnt is 0 */
int dec_ref_fs_vnode(void *node)
{
        /* Lab 5 TODO Begin */

        UNUSED(node);
        
        /* Lab 5 TODO End */

        return 0;
}