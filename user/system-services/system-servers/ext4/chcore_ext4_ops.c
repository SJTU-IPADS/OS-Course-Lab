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

#include "chcore_ext4_defs.h"
#include "ext4.h"
#include <limits.h>
#include <fcntl.h>
#include <fs_wrapper_defs.h>
#include <fs_vnode.h>
#include <fs_page_cache.h>

struct dirent {
        ino_t d_ino;
        off_t d_off;
        unsigned short d_reclen;
        unsigned char d_type;
#define DT_DIR 4
#define DT_REG 8
        char d_name[256];
};

/*
 * Mount
 */
int init_device(void)
{
        const char *mount_point;
        int r;

        /* Dummy arguments for lwext4 lib */
        mount_point = "/";
        file_dev_name_set("ext4.dd");
        if ((bd = file_dev_get()) == NULL) {
                printf("[ext4] error happened in file_dev_get()\n");
                return -EXIT_FAILURE;
        }
        ext4_dmask_set(DEBUG_ALL);

        if (ext4_device_register(bd, "ext4_fs") != EOK) {
                printf("[ext4] error happened in ext4_device_register()\n");
                return -EXIT_FAILURE;
        }
        if (ext4_mount("ext4_fs", mount_point, false) != EOK) {
                printf("[ext4] error happended in ext4_mount()\n");
                return -EXIT_FAILURE;
        }
        r = ext4_recover(mount_point);
        if (r != EOK && r != ENOTSUP) {
                printf("[ext4] error happended in ext4_recover()\n");
                return -EXIT_FAILURE;
        }
        if (ext4_journal_start(mount_point) != EOK) {
                printf("[ext4] error happened in ext4_journal_start()\n");
                return -EXIT_FAILURE;
        }
        if (ext4_cache_write_back(mount_point, 1) != EOK) {
                printf("[ext4] error happened in ext4_cache_write_back()\n");
                return -EXIT_FAILURE;
        }

        return 0;
}

/* set partition_lba */
int __ext4_srv_mount(struct fs_request *fr)
{
        ext4_srv_dbg_base("do_mount, partition_lba=%d\n", fr->offset);

        partition_lba = fr->mount.offset;

        if (init_device() != 0) {
                return -EXIT_FAILURE;
        }

        return 0;
}

int ext4_srv_mount(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        return __ext4_srv_mount(fr);
}

/*
 * Umount
 */

int umount_device(void)
{
        const char *mount_point = "/";

        if (ext4_cache_write_back(mount_point, 0) != EOK) {
                printf("[ext4] error in umount_device ext4_cache_write_back()\n");
                return -EXIT_FAILURE;
        }

        if (ext4_journal_stop(mount_point) != EOK) {
                printf("[ext4] error in umount_device ext4_journal_stop()\n");
                return -EXIT_FAILURE;
        }

        if (ext4_umount(mount_point) != EOK) {
                printf("[ext4] error in umount_device ext4_umount()\n");
                return -EXIT_FAILURE;
        }

        return 0;
}

int __ext4_srv_umount(void)
{
        int ret = umount_device();

        return ret;
}

int ext4_srv_umount(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        return __ext4_srv_umount();
}

/*
 * [IN] path, flags, mode
 * [OUT] vnode_id, vnode_type, vnode_private
 */
int ext4_srv_open(char *path, int flags, int mode, ino_t *vnode_id,
                  off_t *vnode_size, int *vnode_type, void **vnode_private)
{
        int r;
        int type;
        ext4_dir *dir;
        ext4_file *f;
        int ret = 0;

        uint8_t path_start_with_backslash = 1;
        if(path && path[0] != '/'){
                path_start_with_backslash = 0;
                char* new_path;
                new_path = (char*)malloc(strlen(path) + 2);
                strcpy(new_path + 1, path);
                new_path[0] = '/';
                new_path[strlen(path) + 1] = '\0';
                path = new_path;
        }

        /* operator should have both R/W perm */
        flags = (flags & ~(u32)O_WRONLY) | O_RDWR;

        type = ext4_get_inode_type(path);
        if (type == -1 && !(flags & O_CREAT)) {
                ret = -ENOENT;
                goto out;
        }

        if (type == EXT4_DE_REG_FILE || type == -1) {
                f = (ext4_file *)malloc(sizeof(ext4_file));
                if (f == NULL) {
                        ret = -ENOMEM;
                        goto out;
                }
                r = ext4_fopen2(f, path, flags);
                if (r != EOK) {
                        printf("[ext4 fopen error %d\n", r);
                        free(f);
                        ret = -r;
                        goto out;
                }
                *vnode_id = (ino_t)f->inode;
                *vnode_type = FS_NODE_REG;
                *vnode_size = f->fsize;
                *vnode_private = f;
        } else if (type == EXT4_DE_DIR) {
                dir = (ext4_dir *)malloc(sizeof(ext4_dir));
                if (dir == NULL) {
                        ret = -ENOMEM;
                        goto out;
                }
                r = ext4_dir_open(dir, path);
                if (r != EOK) {
                        printf("[ext4] opendir error %d\n", r);
                        free(dir);
                        ret = -r;
                        goto out;
                }
                *vnode_id = dir->f.inode;
                *vnode_type = FS_NODE_DIR;
                *vnode_size = 0; /* TODO: dir size */
                *vnode_private = dir;
        } else {
                BUG_ON(1);
        }
out:
        if(!path_start_with_backslash){
                free(path);
        }
        return ret;
}

int ext4_srv_close(void *operator, bool is_dir, bool do_close)
{
        int ret;

        if (is_dir) {
                ret = ext4_dir_close((ext4_dir *)operator);
                if (ret != EOK) {
                        fs_debug_error("ext4_dir_close failed %d\n", ret);
                        return ret;
                }
        } else {
                ret = ext4_fclose((ext4_file *)operator);
                if (ret != EOK) {
                        fs_debug_error("ext4_fclose failed %d\n", ret);
                        return ret;
                }
        }
        free(operator);

        return 0;
}

/* read(fd, buf, size), return size(read) */
ssize_t ext4_srv_read(void *operator, off_t offset, size_t size, char * buf)
{
        int ret;
        size_t rcnt = 0;

        ext4_file *fp = (ext4_file *)operator;

        /*
         * We shared different fd to same inode with a same FIL struct
         * 	so we should adjust fptr first, then read.
         */
        pthread_mutex_lock(&ext4_meta_lock);
        if (offset < fp->fsize) {
                ext4_fseek(fp, offset, SEEK_SET);
        } else {
                ext4_fseek(fp, 0, SEEK_END);
        }
        ret = ext4_fread(fp, buf, size, &rcnt);
        pthread_mutex_unlock(&ext4_meta_lock);

        if (ret != 0) {
                printf("[ERROR] ext4_read failed %d\n", ret);
                return 0;
        }

        if (rcnt <= LONG_MAX)
                return (ssize_t)rcnt;
        else
                BUG_ON("rcnt larger than ssize_t");
        return -1;
}

/* write(fd, buf, size), return size(write) */
ssize_t ext4_srv_write(void *operator, off_t offset, size_t size,
                       const char * buf)
{
        int ret;
        size_t wcnt = 0;
        ext4_file *fp = (ext4_file *)operator;

        /* Similar with read operation */
        pthread_mutex_lock(&ext4_meta_lock);
        if (offset < fp->fsize) {
                ext4_fseek(fp, offset, SEEK_SET);
        } else {
                ext4_ftruncate(fp, offset);
                ext4_fseek(fp, 0, SEEK_END);
        }
        ret = ext4_fwrite(fp, buf, size, &wcnt);
        pthread_mutex_unlock(&ext4_meta_lock);

        if (ret != 0) {
                printf("[ERROR] ext4_write failed\n");
                return 0;
        }

        if (wcnt <= LONG_MAX)
                return (ssize_t)wcnt;
        else
                BUG_ON("wcnt larger than ssize_t");
        return -1;
}

/* ftruncate */
int ext4_srv_ftruncate(void *operator, off_t len)
{
        int ret;
        ext4_file *fp;
        uint64_t original_size;
        uint64_t tmp_target;
        char zero_block[EXT4_INODE_BLOCK_SIZE];
        size_t wcnt;

        fp = (ext4_file *)operator;

        /*
         * Notice that ext4_ftruncate can only shrink file size,
         * 	but cannot expand file size.
         * We manually expand file size by ext4_fwrite.
         */
        original_size = ext4_fsize(fp);

        if (original_size < len) {
                /* Expand */
                ext4_fseek(fp, 0, SEEK_END);
                memset(zero_block, 0, EXT4_INODE_BLOCK_SIZE);
                while (original_size < len) {
                        tmp_target = MIN(len,
                                         ROUND_UP(original_size + 1,
                                                  EXT4_INODE_BLOCK_SIZE));
                        ext4_fwrite(fp,
                                    zero_block,
                                    tmp_target - original_size,
                                    &wcnt);
                        if (wcnt != tmp_target - original_size)
                                return -1;
                        original_size = tmp_target;
                }
        } else {
                /* Shrink */
                ret = ext4_ftruncate((ext4_file *)operator, len);
                if (ret != EOK)
                        return -1;
        }

        return 0;
}

int ext4_srv_fstatat(const char *path, struct stat *st, int flags);

/* unlink */
int ext4_srv_unlink(const char *path, int flags)
{
        int ret;
        int type;

        if ((ret = check_path_prefix_is_dir(path)) != 0)
                return ret;

        type = ext4_get_inode_type(path);
        if (type == -1)
                return -ENOENT;
        if (type == EXT4_DE_DIR)
                return -EISDIR;

        /* NOTE: ext4_xxx return positive POSIX errno */
        ret = -ext4_fremove(path);

        return ret;
}

/* rmdir */
int ext4_srv_rmdir(const char *path, int flags)
{
        int ret;
        int type;

        if ((ret = check_path_prefix_is_dir(path)) != 0)
                return ret;

        type = ext4_get_inode_type(path);
        if (type == -1)
                return -ENOENT;
        if (type == EXT4_DE_REG_FILE)
                return -ENOTDIR;

        if ((ret = ext4_dir_is_empty(path)) != 0)
                return ret;

        /* NOTE: ext4_xxx return positive POSIX errno */
        ret = -ext4_dir_rm(path);

        return ret;
}

/* fstatat(path) */
int ext4_srv_fstatat(const char *path, struct stat *st, int flags)
{
        int ret;

        ret = check_path_prefix_is_dir(path);
        if (ret) {
                return ret;
        }

        ret = fillstat(st, path);
        return ret;
}

int __ext4_srv_fstat(int fd, struct stat *st)
{
        int ret;
        ret = fillstat(st, (char *)server_entrys[fd]->path);
        return ret;
}

/* fstat(fd) */
int ext4_srv_fstat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int fd = fr->stat.fd;
        struct stat *st = (struct stat *)ipc_get_msg_data(ipc_msg);

        return __ext4_srv_fstat(fd, st);
}

/* mkdir() */
int ext4_srv_mkdir(const char *path, mode_t mode)
{
        int ret;

        ret = check_path_prefix_is_dir(path);
        if (ret)
                return ret;

        if (ext4_inode_exist(path, EXT4_DE_REG_FILE) == EOK
            || ext4_inode_exist(path, EXT4_DE_DIR) == EOK)
                return -EEXIST;

        /* NOTE: ext4_xxx return positive POSIX error value */
        ret = -ext4_dir_mk(path);
        return ret;
}

/* rename() */
int ext4_srv_rename(const char *old_path, const char *new_path)
{
        int ret;

        /* NOTE: ext4_xxx return positive POSIX errno */
        ret = -ext4_frename(old_path, new_path);

        return ret;
}

/* Return true if fd is NOT valid */
static inline bool fd_type_invalid(int fd, bool isfile)
{
        if (fd < 0 || fd >= MAX_SERVER_ENTRY_NUM)
                return true;
        if (server_entrys[fd] == NULL)
                return true;
        if (isfile && (server_entrys[fd]->vnode->type != FS_NODE_REG))
                return true;
        if (!isfile && (server_entrys[fd]->vnode->type != FS_NODE_DIR))
                return true;
        return false;
}

/* TODO: should merge into fs_wrapper */
int ext4_srv_getdent64(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int fd;
        ext4_dir *dp;
        size_t count;
        struct dirent *dirp;
        void *dirp_end;
        const ext4_direntry *e;
        int bytes_read = 0;

        fd = fr->getdents64.fd;
        if (fd_type_invalid(fd, false)) {
                return -EBADF;
        }

        dp = (ext4_dir *)server_entrys[fd]->vnode->private;
        count = fr->getdents64.count;
        dirp = (struct dirent *)ipc_get_msg_data(ipc_msg);
        dirp_end = (void *)dirp + count;

        while (dirp_end - (void *)dirp >= sizeof(struct dirent)) {
                e = ext4_dir_entry_next(dp);
                if (e == NULL) {
                        break;
                }
                dirp->d_ino = e->inode;
                dirp->d_off = sizeof(struct dirent);
                dirp->d_reclen = sizeof(struct dirent);
                dirp->d_type = (e->inode_type == EXT4_DE_DIR) ? DT_DIR : DT_REG;
                strcpy(dirp->d_name, (char *)e->name);
                bytes_read += sizeof(struct dirent);
                dirp++;
        }
        return bytes_read;
}

int ext4_srv_faccessat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        if ((ext4_inode_exist(fr->faccessat.pathname, EXT4_DE_REG_FILE) == EOK)
            || (ext4_inode_exist(fr->faccessat.pathname, EXT4_DE_DIR) == EOK)) {
                printf("Warning: faccessat always return 0 when the file exists.\n");
                return 0;
        }

        /* File or dir does not exist. */
        return -1;
}

int ext4_srv_fcntl(void *operator, int fd, int cmd, int arg)
{
        return 0;
}

struct fs_server_ops server_ops = {
        .mount = ext4_srv_mount,
        .umount = ext4_srv_umount,
        .open = ext4_srv_open,
        .read = ext4_srv_read,
        .write = ext4_srv_write,
        .close = ext4_srv_close,
        .creat = default_server_operation,
        .unlink = ext4_srv_unlink,
        .mkdir = ext4_srv_mkdir,
        .rmdir = ext4_srv_rmdir,
        .rename = ext4_srv_rename,
        .getdents64 = ext4_srv_getdent64,
        .ftruncate = ext4_srv_ftruncate,
        .fstatat = ext4_srv_fstatat,
        .fstat = ext4_srv_fstat,
        .statfs = default_server_operation,
        .fstatfs = default_server_operation,
        .faccessat = ext4_srv_faccessat,
        .symlinkat = default_server_operation,
        .readlinkat = default_ssize_t_server_operation,
        .fallocate = default_server_operation,
        .fcntl = ext4_srv_fcntl,
        .fmap_get_page_addr = default_fmap_get_page_addr,
};
