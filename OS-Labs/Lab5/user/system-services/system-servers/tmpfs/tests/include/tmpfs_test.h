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

#include "../../defs.h"
#include "../../namei.h"
#define MAX_ENTRIES        256
#define MAX_TEST_FILE_SIZE (1l << 20) /* 1 MB */

/* mapping from a full pathname to a dentry (so to its inode) */
struct namei_record {
        struct string fullpath;
        struct hlist_node node;
        struct dentry *dentry;
        bool has_symlink; /* are there symlinks inside the path */
};