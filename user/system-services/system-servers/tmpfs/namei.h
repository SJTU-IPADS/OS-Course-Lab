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

#ifndef TMPFS_NAMEI_H
#define TMPFS_NAMEI_H

#include "defs.h"

#define MAX_STACK_SIZE 3 /* Maximum number of nested symlinks */
#define MAX_SYM_CNT    10 /* Maximum number of symlinks in a lookup */

/* nd->flags flags */
#define ND_FOLLOW         0x0001 /* follow links at the end */
#define ND_DIRECTORY      0x0002 /* require a directory */
#define ND_EMPTY          0x0004 /* accept empty path */
#define ND_NO_SYMLINKS    0x0008 /* no symlinks during the walk */
#define ND_TRAILING_SLASH 0x0010 /* the path ends with one or more slashes */

struct nameidata {
        struct dentry *current;
        struct string last;
        unsigned int flags;
        int last_type; /* LAST_NORM, LAST_DOT, LAST_DOTDOT */
        unsigned depth;
        int total_link_count;
        const char *stack[MAX_STACK_SIZE]; /* saved pathname when handling
                                              symlinks */
};

int path_parentat(struct nameidata *nd, const char *path, unsigned flags,
                  struct dentry **parent);
int path_lookupat(struct nameidata *nd, const char *path, unsigned flags,
                  struct dentry **dentry);
int path_openat(struct nameidata *nd, const char *path, unsigned open_flags,
                unsigned flags, struct dentry **dentry);

#endif /* TMPFS_NAMEI_H */