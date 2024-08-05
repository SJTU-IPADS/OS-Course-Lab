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

#include "mount_info.h"

struct list_head mount_point_infos;

/* Insert new mount_point at BEGINNING */
struct mount_point_info_node *set_mount_point(const char *path, int path_len,
                                              int fs_cap)
{
        struct mount_point_info_node *n;
        n = (struct mount_point_info_node *)malloc(sizeof(*n));
        BUG_ON(n == NULL);

        BUG_ON(path_len > MAX_MOUNT_POINT_LEN);

        strncpy(n->path, path, path_len);
        n->path[path_len] = '\0';
        n->path_len = path_len;
        n->fs_cap = fs_cap;

        list_add(&n->node, &mount_point_infos);
        return n;
}

static bool mount_point_match(struct mount_point_info_node *mp, char *path,
                              int path_len)
{
        /* Root */
        if (mp->path_len == 1 && path[0] == '/')
                return true;

        /* Others */
        if (strncmp(mp->path, path, mp->path_len) != 0)
                return false;
        if (path_len == mp->path_len)
                return true;
        if (path[mp->path_len] == '/')
                return true;
        return false;
}

/* Return the longest FIRST mount_point */
struct mount_point_info_node *get_mount_point(char *path, int path_len)
{
        struct mount_point_info_node *iter;

        int match_len_max = 0;
        struct mount_point_info_node *matched_fs = NULL;

        for_each_in_list (
                iter, struct mount_point_info_node, node, &mount_point_infos) {
                if (iter->path_len <= path_len && iter->path_len > match_len_max
                    && mount_point_match(iter, path, path_len)) {
                        matched_fs = iter;
                        match_len_max = iter->path_len;
                }
        }

        BUG_ON(matched_fs == NULL);
        return matched_fs;
}

/* Remove the FIRST mount_point matched */
int remove_mount_point(char *path)
{
        struct mount_point_info_node *iter;

        for_each_in_list (
                iter, struct mount_point_info_node, node, &mount_point_infos) {
                if (strcmp(iter->path, path) == 0) {
                        list_del(&iter->node);
                        free(iter);
                        return 0;
                }
        }
        return -1;
}