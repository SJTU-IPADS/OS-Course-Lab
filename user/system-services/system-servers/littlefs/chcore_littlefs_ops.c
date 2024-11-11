#include "bd/lfs_rambd.h"
#include "fs_vnode.h"
#include "fs_wrapper_defs.h"
#include "chcore_littlefs_defs.h"
#include "lfs.h"
#include "stdbool.h"
#include <stddef.h>
#include <chcore/container/rbtree.h>
#include <chcore/syscall.h>
#include <assert.h>
#include <chcore/ipc.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>

static inline unsigned long hash_chars(const char *str, size_t len)
{
        unsigned long seed = 131; /* 31 131 1313 13131 131313 etc.. */
        unsigned long hash = 0;
        int i;

        if (len < 0) {
                while (*str) {
                        hash = (hash * seed) + *str;
                        str++;
                }
        } else {
                for (i = 0; i < len; ++i)
                        hash = (hash * seed) + str[i];
        }

        return hash;
}

struct lfs_opened_node {
        struct rb_node node;
        unsigned long path_hash;
        int type;
        union {
            lfs_file_t *file;
            lfs_dir_t *dir;
        };
};

static int cmp_hash_opened_node(const void *key, const struct rb_node *node)
{
        const unsigned long *hash = key;
        const struct lfs_opened_node *file =
                rb_entry(node, struct lfs_opened_node, node);

        if (*hash < file->path_hash)
                return -1;
        else if (*hash > file->path_hash)
                return 1;
        else
                return 0;
}

static bool less_opened_node(const struct rb_node *a, const struct rb_node *b)
{
        const struct lfs_opened_node *file_a =
                rb_entry(a, struct lfs_opened_node, node);
        const struct lfs_opened_node *file_b =
                rb_entry(b, struct lfs_opened_node, node);

        return file_a->path_hash < file_b->path_hash;
}

static struct rb_root lfs_opened_nodes;

static lfs_t lfs;

void init_littlefs()
{
        int ret;
        ret = init_bd();
        if (ret < 0) {
                printf("[LittleFS Driver]: init block device failed, ret = %d\n", ret);
                usys_exit(ret);
        }
        init_rb_root(&lfs_opened_nodes);
        mounted = false;
        init_fs_wrapper();
        using_page_cache = false;
}

int chcore_lfs_mount(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int ret;

        if (bd_cfg.context)
                lfs_rambd_create(&bd_cfg);

        ret = lfs_mount(&lfs, &bd_cfg);
        if (ret < 0) {
                lfs_format(&lfs, &bd_cfg);
                ret = lfs_mount(&lfs, &bd_cfg);
        }

        return ret;
}

int chcore_lfs_umount(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        return lfs_unmount(&lfs);
}

static int translate_posix_flags(int flags)
{
        int ret = 0;

        ret |= (flags & O_RDONLY) ? LFS_O_RDONLY : 0;
        ret |= (flags & O_WRONLY) ? LFS_O_WRONLY : 0;
        ret |= (flags & O_RDWR) ? LFS_O_RDWR : 0;
        ret |= (flags & O_CREAT) ? LFS_O_CREAT : 0;
        ret |= (flags & O_EXCL) ? LFS_O_EXCL : 0;
        ret |= (flags & O_TRUNC) ? LFS_O_TRUNC : 0;
        ret |= (flags & O_APPEND) ? LFS_O_APPEND : 0;

        return ret;
}

int chcore_lfs_open(char *path, int flags, int mode, ino_t *vnode_id, off_t *vnode_size, int *vnode_type, void **private)
{
        int ret;
        unsigned long path_hash;
        struct rb_node *node;
        struct lfs_opened_node *opened_node;
        struct lfs_info info;
        bool is_create = false;

        path_hash = hash_chars(path, strlen(path));

        node = rb_search(&lfs_opened_nodes, &path_hash, cmp_hash_opened_node);

        if (node) {
                ret = -EEXIST;
                goto out;
        }

        ret = lfs_stat(&lfs, path, &info);
        if (ret < 0 && ret != LFS_ERR_NOENT) {
                goto out;
        } else if (ret == LFS_ERR_NOENT && !(flags & O_CREAT)) {
                ret = -ENOENT;
                goto out;
        } else if (ret == LFS_ERR_NOENT && (flags & O_CREAT)) {
                is_create = true;
        }

        opened_node = malloc(sizeof(*opened_node));
        opened_node->path_hash = path_hash;
        flags = translate_posix_flags(flags);
        
        if (info.type == LFS_TYPE_REG || is_create) {
                opened_node->file = malloc(sizeof(*opened_node->file));
                ret = lfs_file_open(&lfs, opened_node->file, path, flags);
                opened_node->type = LFS_TYPE_REG;
                *vnode_type = FS_NODE_REG;
                if (is_create) {
                        lfs_stat(&lfs, path, &info);
                }
                *vnode_size = info.size;
        } else if (info.type == LFS_TYPE_DIR) {
                opened_node->dir = malloc(sizeof(*opened_node->dir));
                ret = lfs_dir_open(&lfs, opened_node->dir, path);
                opened_node->type = LFS_TYPE_DIR;
                *vnode_type = FS_NODE_DIR;
                *vnode_size = 0;
        } else {
                ret = -EINVAL;
                goto out;
        }
        *vnode_id = path_hash;
        *private = opened_node;
        rb_insert(&lfs_opened_nodes, &opened_node->node, less_opened_node);
out:
        return ret;
}

int chcore_lfs_close(void *operator, bool is_dir, bool do_close)
{
        int ret;
        struct lfs_opened_node *opened_node = operator;

        rb_erase(&lfs_opened_nodes, &opened_node->node);
        if (is_dir) {
                assert(opened_node->type == LFS_TYPE_DIR);
                ret = lfs_dir_close(&lfs, opened_node->dir);
                free(opened_node->dir);
        } else {
                assert(opened_node->type == LFS_TYPE_REG);
                ret = lfs_file_close(&lfs, opened_node->file);
                free(opened_node->file);
        }

        free(opened_node);


        return ret;
}

ssize_t chcore_lfs_read(void *operator, off_t offset, size_t size, char *buf)
{
        int ret;
        struct lfs_opened_node *opened_node = operator;

        assert(opened_node->type == LFS_TYPE_REG);
        if (offset + size > UINT32_MAX) {
                return -EINVAL;
        }

        if (opened_node->file->off != offset) {
                ret = lfs_file_seek(&lfs, opened_node->file, (lfs_off_t)offset, LFS_SEEK_SET);
                if (ret < 0) {
                        return ret;
                }
        }

        ret = lfs_file_read(&lfs, opened_node->file, buf, size);

        return ret;
}

ssize_t chcore_lfs_write(void *operator, off_t offset, size_t size, const char *buf)
{
        int ret;
        struct lfs_opened_node *opened_node = operator;

        assert(opened_node->type == LFS_TYPE_REG);
        if (offset + size > UINT32_MAX) {
                return -EINVAL;
        }

        if (opened_node->file->off != offset) {
                ret = lfs_file_seek(&lfs, opened_node->file, (lfs_off_t)offset, LFS_SEEK_SET);
                if (ret < 0) {
                        return ret;
                }
        }

        ret = lfs_file_write(&lfs, opened_node->file, buf, size);

        return ret;
}

int chcore_lfs_unlink(const char *path, int flags)
{
        return lfs_remove(&lfs, path);
}

int chcore_lfs_mkdir(const char *path, mode_t mode)
{
        return lfs_mkdir(&lfs, path);
}

extern int lfs_rawremove(lfs_t *lfs, const char *path);

extern int lfs_rawstat(lfs_t *lfs, const char *path, struct lfs_info *info);

int chcore_lfs_rmdir(const char *path, int flags)
{
        int ret = 0;
        struct lfs_info info;
        lfs.cfg->lock(&bd_cfg);
        ret = lfs_rawstat(&lfs, path, &info);
        if (info.type != LFS_TYPE_DIR) {
                ret = -ENOTDIR;
                goto out;
        }

        ret = lfs_rawremove(&lfs, path);
out:
        lfs.cfg->unlock(&bd_cfg);
        return ret;
}

int chcore_lfs_rename(const char *oldpath, const char *newpath)
{
        return lfs_rename(&lfs, oldpath, newpath);
}

int chcore_lfs_ftruncate(void *operator, off_t size)
{
        int ret;
        struct lfs_opened_node *opened_node = operator;

        assert(opened_node->type == LFS_TYPE_REG);

        if (size > UINT32_MAX) {
                return -EFBIG;
        }

        ret = lfs_file_truncate(&lfs, opened_node->file, (lfs_off_t)size);

        return ret;
}

static int fill_stat_from_path(const char *path, struct stat *st)
{
        int ret;
        struct lfs_info info;

        ret = lfs_stat(&lfs, path, &info);
        if (ret < 0) {
                return ret;
        }

        st->st_ino = hash_chars(path, strlen(path));
        st->st_size = info.size;
        st->st_mode = info.type == LFS_TYPE_REG ? S_IFREG : S_IFDIR;
        st->st_nlink = 1;
        st->st_uid = 0;
	st->st_gid = 0;

	st->st_rdev = 0;
        st->st_blksize = lfs.cfg->block_size;
        st->st_blocks = 0;


        return 0;
}

int chcore_lfs_fstatat(const char *path, struct stat *st, int flags)
{
        int ret;
        ret = fill_stat_from_path(path, st);
        return ret;
}

int chcore_lfs_fstat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int fd = fr->stat.fd, ret;
	struct stat *st = (struct stat *)ipc_get_msg_data(ipc_msg);

        ret = fill_stat_from_path((char *)server_entrys[fd]->path, st);
        return ret;
}

static size_t fill_dirent_from_info(struct lfs_info *info, struct dirent *dirent)
{
        size_t len;

        dirent->d_ino = hash_chars(info->name, strlen(info->name));
        dirent->d_type = info->type == LFS_TYPE_REG ? DT_REG : DT_DIR;
        len = strlen(info->name);
        strncpy(dirent->d_name, info->name, 256);
        dirent->d_name[len] = '\0';
        dirent->d_reclen = sizeof(dirent->d_off) + sizeof(dirent->d_type) + sizeof(dirent->d_ino) + sizeof(dirent->d_reclen) + len + 1;
        dirent->d_reclen = ROUND_UP(dirent->d_reclen, 8);

        return dirent->d_reclen;
}

int chcore_lfs_getdents64(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int fd = fr->getdents64.fd, ret;
        char *dirent_buf = ipc_get_msg_data(ipc_msg);
        
        struct dirent tmp_dent;
        struct lfs_opened_node *opened_node = server_entrys[fd]->vnode->private;
        size_t start_lfs_dir_offset = server_entrys[fd]->offset, cur_lfs_dir_offset, filled_bytes;
        size_t count = fr->getdents64.count;
        struct lfs_info info;
        struct dirent *cur_dirent = (struct dirent *)dirent_buf;
        int ret_filled_bytes = 0;

        char *dirent_buf_end = dirent_buf + count;

        assert(opened_node->type == LFS_TYPE_DIR);

        for (cur_lfs_dir_offset = start_lfs_dir_offset; 
                dirent_buf < dirent_buf_end; cur_lfs_dir_offset++) {
                ret = lfs_dir_read(&lfs, opened_node->dir, &info);
                if (ret < 0) {
                        goto out;
                } else if (ret == 0) {
                        break;
                }

                filled_bytes = fill_dirent_from_info(&info, &tmp_dent);
                if (dirent_buf + filled_bytes > dirent_buf_end) {
                        break;
                }
                memcpy(cur_dirent, &tmp_dent, filled_bytes);
                ret_filled_bytes += filled_bytes;
                dirent_buf += filled_bytes;
                cur_dirent = (struct dirent *)dirent_buf;
        }
        
        server_entrys[fd]->offset += cur_lfs_dir_offset - start_lfs_dir_offset;
        return ret_filled_bytes;
out:
        return ret;
}

int chcore_lfs_fcntl(void *operator, int fd, int fcntl_cmd, int fcntl_arg)
{
        return 0;
}

struct fs_server_ops server_ops = {
        .mount = chcore_lfs_mount,
        .umount = chcore_lfs_umount,

        .open = chcore_lfs_open,
        .read = chcore_lfs_read,
        .write = chcore_lfs_write,
        .close = chcore_lfs_close,

        .creat = default_server_operation,
        .unlink = chcore_lfs_unlink,
        .mkdir = chcore_lfs_mkdir,
        .rmdir = chcore_lfs_rmdir,
        .rename = chcore_lfs_rename,

        .getdents64 = chcore_lfs_getdents64,
        .ftruncate = chcore_lfs_ftruncate,
        .fstatat = chcore_lfs_fstatat,
        .fstat = chcore_lfs_fstat,
        .statfs = default_server_operation,
        .fstatfs = default_server_operation,
        .faccessat = default_server_operation,

        .symlinkat = default_server_operation,
        .readlinkat = default_ssize_t_server_operation,
        .fallocate = default_server_operation,
        .fcntl = chcore_lfs_fcntl
};