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

#include <chcore-internal/fs_defs.h>
#include "chcore_ext4_defs.h"
#include "chcore_ext4_server.h"
#include <fs_vnode.h>

/*
 * ext4 wrapper using libext4
 */

/* construct struct stat *st */
int fillstat(struct stat *st, const char *path)
{
        int ret;
        ext4_file f;
        ext4_dir d;

        st->st_dev = 0;

        if (ext4_inode_exist(path, EXT4_DE_REG_FILE) == EOK) {
                /* regular file */
                ret = ext4_fopen(&f, path, "r");
                if (ret == EOK) {
                        st->st_mode = S_IFREG;
                        st->st_size = f.fsize;
                        st->st_ino = (ino_t)f.inode;
                        ret = ext4_fclose(&f);
                        BUG_ON(ret);
                } else {
                        BUG_ON(ret);
                        goto err;
                }

        } else if (ext4_inode_exist(path, EXT4_DE_DIR) == EOK) {
                ret = ext4_dir_open(&d, path);
                if (ret == EOK) {
                        st->st_mode = S_IFDIR;
                        st->st_ino = (ino_t)d.f.inode;
                        ret = ext4_dir_close(&d);
                        BUG_ON(ret);
                } else {
                        BUG_ON(ret);
                        goto err;
                }
        } else {
                goto err;
        }

        st->st_nlink = 1;
        /* TODO: uid/gid. */
        st->st_uid = 0;
        st->st_gid = 0;

        st->st_rdev = 0;

        ext4_atime_get(path, (uint32_t *)&(st->st_atime));
        ext4_mtime_get(path, (uint32_t *)&(st->st_mtime));
        ext4_ctime_get(path, (uint32_t *)&(st->st_ctime));

        /* TODO: Handle the following two. */
        st->st_blksize = 0;
        st->st_blocks = 0;

        return 0;
err:
        memset(st, 0, sizeof(*st));
        return -ENOENT;
}

void check_r(int condition, char *_file, int _line)
{
        if (!condition) {
                ext4_dbg("[Error] Assersion Failed __FILE__=%s __LINE__=%d\n",
                         _file,
                         _line);
                while (1)
                        ;
        }
        ext4_dbg("check_r called: %s %d\n", _file, _line);
}

/* TODO: path should be a normalized path */
static int get_path_prefix(const char *path, char *path_prefix)
{
        int i;
        int ret;

        ret = -1; /* return -1 means find no '/' */

        BUG_ON(strlen(path) > EXT4_PATH_LEN_MAX);

        strcpy(path_prefix, path);
        for (i = strlen(path_prefix) - 2; i >= 0; i--) {
                if (path_prefix[i] == '/') {
                        path_prefix[i] = '\0';
                        ret = 0;
                        break;
                }
        }

        return ret;
}

static int get_path_leaf(const char *path, char *path_leaf)
{
        int i;
        int ret;

        ret = -1; /* return -1 means find no '/' */

        for (i = strlen(path) - 2; i >= 0; i--) {
                if (path[i] == '/') {
                        strcpy(path_leaf, path + i + 1);
                        ret = 0;
                        break;
                }
        }

        if (ret == -1)
                return ret;

        if (path_leaf[strlen(path_leaf) - 1] == '/') {
                path_leaf[strlen(path_leaf) - 1] = '\0';
        }

        return ret;
}

int ext4_get_inode_type(const char *path)
{
        if (ext4_inode_exist(path, EXT4_DE_REG_FILE) == EOK)
                return EXT4_DE_REG_FILE;
        if (ext4_inode_exist(path, EXT4_DE_DIR) == EOK)
                return EXT4_DE_DIR;
        return -1; /* not exists */
}

int ext4_dir_is_empty(const char *path)
{
        ext4_dir dir;
        const ext4_direntry *e;
        int ret;

        ret = ext4_dir_open(&dir, path);
        BUG_ON(ret);
        while ((e = ext4_dir_entry_next(&dir)) != NULL) {
                if (strncmp((char *)e->name, ".", e->name_length)
                    && strncmp((char *)e->name, "..", e->name_length)) {
                        ret = -ext4_dir_close(&dir);
                        BUG_ON(ret);
                        return -ENOTEMPTY;
                }
        }

        /* new is empty dir */
        ret = -ext4_dir_close(&dir);
        BUG_ON(ret);

        return 0;
}

int check_path_prefix_is_dir(const char *path)
{
        char path_prefix[EXT4_PATH_BUF_LEN];

        if (get_path_prefix(path, path_prefix) == -1)
                return -EINVAL;

        if (path_prefix[0]) {
                if (ext4_inode_exist(path_prefix, EXT4_DE_REG_FILE) == EOK)
                        return -ENOTDIR;
                if (ext4_inode_exist(path_prefix, EXT4_DE_DIR) != EOK)
                        return -ENOENT; /* path_prefix not exists */
        }

        return 0;
}

int check_path_leaf_is_not_dot(const char *path)
{
        char leaf[EXT4_PATH_BUF_LEN];

        if (get_path_leaf(path, leaf) == -1)
                return -EINVAL;
        if (strcmp(leaf, ".") == 0 || strcmp(leaf, "..") == 0)
                return -EINVAL;

        return 0;
}
