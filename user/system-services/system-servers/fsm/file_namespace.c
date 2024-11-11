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

#include "file_namespace.h"
#include <chcore/error.h>
#include <fcntl.h>
#include <chcore/file_namespace.h>

pthread_rwlock_t fsm_client_namespace_table_rwlock;
struct list_head fsm_client_namespace_table;

struct fsm_client_namespace_node {
        badge_t client_badge;

        struct file_namespace *file_ns;

        struct list_head node;
};

struct file_info_node {
        char path[FS_NS_MAX_PATH_LEN + 1];
        size_t path_len;
        char mount_path[FS_NS_MAX_PATH_LEN + 1]; //path in the whole content tree
        size_t mount_path_len;
        int flags;

        struct list_head node;
};

static int __set_file_namespace(badge_t client_badge, struct file_namespace *ns);
static int __set_file_node_info(struct file_namespace *file_ns, struct user_file_info_node *unode);
static void __init_file_namespace(struct file_namespace *file_ns);

size_t __file_info_match(struct file_info_node *pn, const char *path, int path_len)
{
        size_t t_begin, t_end, t_size;
        size_t p_begin, p_end, p_size;
        
        t_end = 0;
        p_end = 0;
        while (true) {
                t_begin = t_end + strspn(pn->path + t_end, "/");
                p_begin = p_end + strspn(path + p_end, "/");
                if (t_begin == pn->path_len)
                        return p_begin;
                if (p_begin == path_len)
                        return 0;
                t_size = strcspn(pn->path + t_begin, "/");
                p_size = strcspn(path + p_begin, "/");
                if (t_size != p_size)
                        return 0;
                if (strncmp(pn->path + t_begin, path + p_begin, p_size))
                        return 0;
                t_end = t_begin + t_size;
                p_end = p_begin + p_size;
                if (t_end == pn->path_len)
                        return p_end;
                if (p_end == path_len)
                        return 0;
        }
}

int fsm_delete_file_namespace(badge_t client_badge)
{
        struct fsm_client_namespace_node *n = NULL, *iter;
        int ret = 0;

        /* get namespace */
        pthread_rwlock_wrlock(&fsm_client_namespace_table_rwlock);
        for_each_in_list (iter,
                          struct fsm_client_namespace_node,
                          node,
                          &fsm_client_namespace_table) {
                if (iter->client_badge == client_badge) {
                        n = iter;
                        break;
                }
        }

        if (n == NULL) {
                ret = -ENOENT;
                goto out;
        }

        /* delete file namespace */
        if (n->file_ns) {
                n->file_ns->refcount -= 1;
                if (n->file_ns->refcount == 0)
                        free(n->file_ns);
        }

        /* delete namespace node */
        list_del(&n->node);
        free(n);

out:

        pthread_rwlock_unlock(&fsm_client_namespace_table_rwlock);
        return ret;
}

struct file_namespace *fsm_get_file_namespace(badge_t client_badge)
{
        struct fsm_client_namespace_node *n;

        for_each_in_list (
                n, struct fsm_client_namespace_node, node, &fsm_client_namespace_table)
                if (n->client_badge == client_badge)
                        return n->file_ns;

        return NULL;
}

int fsm_new_file_namespace(badge_t client_badge, badge_t parent_badge,
                           int with_config, struct user_file_namespace *fs_ns)
{
        struct file_namespace *parent_file_ns = NULL, *file_ns = NULL;
        int ret = 0;
        struct user_file_namespace user_ns;

        /* get parent namespace */
        pthread_rwlock_wrlock(&fsm_client_namespace_table_rwlock);
        parent_file_ns = fsm_get_file_namespace(parent_badge);
        if (parent_file_ns && with_config != 0) {
                /*
                 * XXX: Haven't supported creating child proccess in parent process which
                 * already has file namespace.
                 */
                ret = -EINVAL;
                goto out;
        }

        /* if has, read namespace config */
        if (with_config != 0) {
                memcpy((void *)&user_ns, fs_ns, sizeof(struct user_file_namespace));
                file_ns = malloc(sizeof(struct file_namespace));
                if (file_ns == NULL) {
                        ret = -ENOMEM;
                        goto out;
                }
                __init_file_namespace(file_ns);

                for (int i = 0; i < user_ns.nums && i < FS_NS_MAX_MOUNT_NUM; i++) {
                        ret = __set_file_node_info(file_ns, &user_ns.file_infos[i]);
                        if (ret < 0) {
                                goto out_free_file_ns_if_fail;
                        }
                }
        } else {
                file_ns = parent_file_ns;
        }

        /* set current process namespace */
        if (file_ns)
                ret = __set_file_namespace(client_badge, file_ns);

out_free_file_ns_if_fail:
        if (ret < 0 && file_ns != parent_file_ns)
                free(file_ns);
out:
        pthread_rwlock_unlock(&fsm_client_namespace_table_rwlock);

        return ret;
}

int parse_path_from_file_namespace(struct file_namespace *file_ns, const char *path,
                                   int flags, char *full_path,
                                   size_t full_path_len)
{
        struct file_info_node *iter;
        size_t path_len, max_prefix_len = 0, prefix_len;
        struct file_info_node *pinfo = NULL;

        /* Ignore extra '/' */
        if (path[0] == '/')
                path += strspn(path, "/") - 1;
        path_len = strlen(path);

        for_each_in_list (iter, struct file_info_node, node, &file_ns->file_infos) {
                prefix_len = __file_info_match(iter, path, path_len);
                if (prefix_len > max_prefix_len) {
                        pinfo = iter;
                        max_prefix_len = prefix_len;
                }
        }
        if (pinfo == NULL)
                return -ENOENT;
        /* Check the file ns mode */
        if (flags == MNT_READWRITE && pinfo->flags == MNT_READONLY)
                return -EROFS;

        strncpy(full_path, pinfo->mount_path, full_path_len);
        strncat(full_path, path + max_prefix_len, full_path_len - strlen(full_path));
        full_path[full_path_len] = '\0';

        return 0;
}

static int __set_file_namespace(badge_t client_badge, struct file_namespace *ns)
{
        struct fsm_client_namespace_node *n;

        n = (struct fsm_client_namespace_node *)malloc(sizeof(*n));
        if (n == NULL) {
                return -ENOMEM;
        }
        n->client_badge = client_badge;
        n->file_ns = ns;
        ns->refcount += 1;

        list_append(&n->node, &fsm_client_namespace_table);

        return 0;
}

/* Insert new file_info at BEGINNING */
int __set_file_node_info(struct file_namespace *file_ns, struct user_file_info_node *unode)
{
        struct file_info_node *n;

        /* check args from user */
        if (unode->path_len > FS_NS_MAX_PATH_LEN || unode->mount_path_len > FS_NS_MAX_PATH_LEN)
                return -ENAMETOOLONG;

        n = (struct file_info_node *)malloc(sizeof(*n));
        if (n == NULL)
                return -ENOMEM;

        memset(n, 0, sizeof(*n));
        memcpy(n->path, unode->path, unode->path_len);
        n->path_len = strlen(n->path);
        memcpy(n->mount_path, unode->mount_path, unode->mount_path_len);
        n->mount_path_len = strlen(n->mount_path);
        n->flags = unode->flags;

        list_add(&n->node, &file_ns->file_infos);
        return 0;
}

void __init_file_namespace(struct file_namespace *file_ns)
{
        init_list_head(&file_ns->file_infos);
        pthread_rwlock_init(&file_ns->file_infos_rwlock, NULL);
        file_ns->refcount = 0;
}

int fs2MntFlag(int fs_flags)
{
        if ((fs_flags & 0xf) > 0)
                return MNT_READWRITE;
        else
                return MNT_READONLY;
}
