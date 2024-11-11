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

#include <bits/errno.h>
#include <chcore/type.h>
#include <chcore/ipc.h>
#include <chcore/bug.h>
#include <chcore/syscall.h>
#include <pthread.h>
#include <malloc.h>
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <string.h>

#include <chcore-internal/fs_defs.h>
#include <chcore-internal/fs_debug.h>
#include <fs_wrapper_defs.h>
#include <fs_vnode.h>
#include <fs_page_cache.h>
#include "ff.h"
#include "diskio.h"
#include "fat32_defs.h"
#include "fat32_utils.h"

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

int fat_open(char *path, int flags, int mode, ino_t *vnode_id,
             off_t *vnode_size, int *vnode_type, void **vnode_private)
{
        int ff_mode;
        int ret = 0;
        FIL *fil;
        DIR *dir;

        if (strlen(path) == 0) {
                /* opendir root */
                path = "/";
        }

        if (path[strlen(path) - 1] == '/') {
                dir = (DIR *)malloc(sizeof(*dir));
                ret = f_opendir(dir, path);
                if (ret) {
                        fs_debug_error("fat opendir failed %s\n", path);
                        free(dir);
                        return ret;
                }

                *vnode_id = DIR_INO(dir);
                *vnode_type = FS_NODE_DIR;
                *vnode_size = 0; /* TODO: dir's size */
                *vnode_private = dir;
                return 0;
        }

        ff_mode = flags_posix2internal(flags);

        /* Try open as regular file */
        fil = (FIL *)malloc(sizeof(*fil));
        ret = f_open(fil, (const TCHAR *)path, ff_mode);
        if (FR_OK != ret) {
                /* path is not a regular file, but maybe a directory */
                free(fil);
                if (FR_NO_FILE == ret) {
                        /* Try open as dir */
                        dir = (DIR *)malloc(sizeof(*dir));
                        ret = f_opendir(dir, path);
                        if (ret) {
                                fs_debug_error("fat opendir failed %s\n", path);
                                free(dir);
                                return -ret;
                        }

                        *vnode_id = DIR_INO(dir);
                        *vnode_type = FS_NODE_DIR;
                        *vnode_size = 0; /* TODO: dir's size */
                        *vnode_private = dir;
                        return 0;
                }
                return -ENOENT;
        }

        /* Do open and get inode number */
        *vnode_id = FILE_INO(fil);
        *vnode_type = FS_NODE_REG;
        *vnode_size = fil->obj.objsize;
        *vnode_private = fil;
        return 0;
}

ssize_t fat_read(void *operator, off_t offset, size_t size, char * buf)
{
        FIL *fp;
        UINT ret = 0;

        if (offset + size > UINT32_MAX) {
                return -EINVAL;
        }

        fp = (FIL *)operator;

        /*
         * We shared different fd to same inode with a same FIL struct
         * 	so we should adjust fptr first, then read.
         */
        pthread_mutex_lock(&fat_meta_lock);
        if (offset < fp->obj.objsize) {
                f_lseek(fp, (FSIZE_t)offset);
        } else {
                f_lseek(fp, fp->obj.objsize);
        }
        f_read(fp, buf, (UINT)size, &ret);
        pthread_mutex_unlock(&fat_meta_lock);

        BUG_ON(ret > LONG_MAX);
        return (ssize_t)ret;
}

ssize_t fat_write(void *operator, off_t offset, size_t size, const char * buf)
{
        FIL *fp;
        UINT ret = 0;

        if (offset + size > UINT32_MAX) {
                return -EINVAL;
        }

        fp = (FIL *)operator;

        pthread_mutex_lock(&fat_meta_lock);
        if (offset < fp->obj.objsize) {
                f_lseek(fp, (FSIZE_t)offset);
        } else {
                f_lseek(fp, fp->obj.objsize);
        }
        f_write(fp, buf, (UINT)size, &ret);
        pthread_mutex_unlock(&fat_meta_lock);

        BUG_ON(ret > LONG_MAX);
        return (ssize_t)ret;
}

int fat_close(void *operator, bool is_dir, bool do_close)
{
        int ret;
        if (is_dir) {
                ret = f_closedir((DIR *)operator);
                if (ret != FR_OK) {
                        fs_debug_error("fat_close failed %d\n", ret);
                        return ret;
                }
        } else {
                ret = f_close((FIL *)operator);
                if (ret != FR_OK) {
                        fs_debug_error("fat_close failed %d\n", ret);
                        return ret;
                }
        }
        free(operator);

        return 0;
}

int fat_ftruncate(void *operator, off_t len)
{
        int ret = 0;
        FIL *fp;

        if (len > UINT32_MAX) {
                return -EFBIG;
        }

        fp = (FIL *)operator;
        f_lseek(fp, (FSIZE_t)len);
        /* TODO: Use the right errno for truncate. */
        ret = f_truncate(fp);
        return ret;
}

/* TODO: path should be a normalized path */
static int get_path_prefix(const char *path, char *path_prefix)
{
        unsigned long i;
        int ret;

        ret = -1; /* return -1 means find no '/' */

        BUG_ON(strlen(path) > FAT_PATH_LEN_MAX);

        strlcpy(path_prefix, path, FAT_PATH_LEN_MAX);
        for (i = strlen(path_prefix) - 2; i >= 0; i--) {
                if (path_prefix[i] == '/') {
                        path_prefix[i] = '\0';
                        ret = 0;
                        break;
                }
        }

        return ret;
}

int fat_mkdir(const char *path, mode_t mode)
{
        int ret;
        char path_prefix[FAT_PATH_BUF_LEN];
        FILINFO fno;

        if (get_path_prefix(path, path_prefix) == -1)
                return -EINVAL;

        /* Check if path_prefix exists */
        if (path_prefix[0]) {
                ret = f_stat(path_prefix, &fno);
                if (ret == FR_NO_FILE) {
                        return -ENOENT;
                }
        } else {
                /* prefix is '/' */
                ret = FR_OK;
                fno.fattrib = AM_DIR;
        }

        BUG_ON(ret != FR_OK);
        if (fno.fattrib & AM_DIR) {
                ret = f_mkdir(path);
                if (ret == FR_OK)
                        return 0;
                if (ret == FR_EXIST)
                        return -EEXIST;
                BUG_ON("TODO: not handle other ret value");
        } else {
                return -ENOTDIR;
        }

        BUG_ON("never reach here");
        return 0;
}

int fat_fstatat(const char *path, struct stat *st, int flags);

int fat_unlink(const char *path, int flags)
{
        int ret;
        FILINFO fno;
        char path_prefix[FAT_PATH_BUF_LEN];

        /* Check validation of path_prefix */
        if (get_path_prefix(path, path_prefix) == -1)
                return -EINVAL;
        if (path_prefix[0]) {
                ret = f_stat(path_prefix, &fno);
                if (ret == FR_NO_FILE || ret == FR_NO_PATH)
                        return -ENOENT;
                if (!(fno.fattrib & AM_DIR))
                        return -ENOTDIR;
        }

        /* Check validation of path */
        ret = f_stat(path, &fno);
        if (ret == FR_NO_PATH)
                return -ENOTDIR;
        if (ret == FR_NO_FILE)
                return -ENOENT;
        if (ret != FR_OK) {
                printf("unexpected error value %d\n", ret);
                BUG_ON(1);
        }

        /* unlink should apply on regular files */
        if (fno.fattrib & AM_DIR)
                return -EISDIR;

        /* do unlink */
        ret = f_unlink(path);
        if (ret != FR_OK) {
                printf("unexpected error value %d\n", ret);
                BUG_ON(1);
        }

        return 0;
}

int fat_rmdir(const char *path, int flags)
{
        int ret;
        FILINFO fno;

        /* Check path is dir */
        ret = f_stat(path, &fno);
        if (ret == FR_NO_PATH)
                return -ENOTDIR;
        if (ret == FR_NO_FILE)
                return -ENOENT;
        if (ret != FR_OK) {
                printf("unexpected error value %d\n", ret);
                BUG_ON(1);
        }

        if (!(fno.fattrib & AM_DIR))
                return -ENOTDIR;

        ret = f_rmdir(path);
        if (ret == FR_DENIED)
                return -ENOTEMPTY;
        if (ret != FR_OK) {
                printf("unexpected error value %d\n", ret);
                BUG_ON(1);
        }

        return 0;
}

int fat_rename(const char *old_path, const char *new_path)
{
        int ret;

        ret = f_rename(old_path, new_path);
        if (ret != FR_OK) {
                printf("unexpected error value %d\n", ret);
                BUG_ON(1);
        }

        return 0;
}

int fat_fstat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int ret;
        FILINFO fno;
        FIL *fil;

        int fd = fr->stat.fd;
        struct stat *st = (struct stat *)ipc_get_msg_data(ipc_msg);

        /* Flush corresponding dirty data into disk, then do stat */
        fil = (FIL *)server_entrys[fd]->vnode->private;
        f_sync(fil);

        ret = f_stat((char *)server_entrys[fd]->path, &fno);
        fill_stat(st, &fno, (ino_t)FILE_INO(fil));
        if (ret == FR_NO_FILE || ret == FR_NO_PATH)
                st->st_nlink = 0;
        return 0;
}

int fat_fstatat(const char *path, struct stat *st, int flags)
{
        int ret;
        FILINFO fno;
        FIL fil;
        DIR dp;
        char path_prefix[FAT_PATH_BUF_LEN];

        /* TODO: we should traverse ENTRYs and flush corresponding FIL */

        /* Check path_prefix */
        if (get_path_prefix(path, path_prefix) == -1)
                return -1;
        if (path_prefix[0]) {
                ret = f_stat(path_prefix, &fno);
                if (ret == FR_NO_FILE || ret == FR_NO_PATH)
                        return -ENOENT;
                if (ret != FR_OK) {
                        printf("unexpected error value %d\n", ret);
                        BUG_ON(1);
                }
                if (!(fno.fattrib & AM_DIR))
                        return -ENOTDIR;
        }

        /* Check path */
        ret = f_stat(path, &fno);
        if (ret != FR_OK) {
                return -ENOENT;
        }

        if (fno.fattrib & AM_DIR) {
                ret = f_opendir(&dp, path);
                if (ret != FR_OK)
                        return -ENOENT;
                fill_stat(st, &fno, (ino_t)DIR_INO(&dp));
                f_closedir(&dp);
        } else {
                ret = f_open(&fil, path, FA_READ);
                if (ret != FR_OK)
                        return -ENOENT;
                fill_stat(st, &fno, (ino_t)FILE_INO(&fil));
                f_close(&fil);
        }

        return ret;
}

int fat_getdents64(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int ret;
        int fd;
        DIR *dp;
        size_t count;
        struct dirent *dirp;
        void *dirp_end;
        FILINFO finfo;
        int bytes_read = 0;

        fd = fr->getdents64.fd;

        if (fd_type_invalid(fd, false)) {
                return -EBADF;
        }

        dp = (DIR *)server_entrys[fd]->vnode->private;
        count = fr->getdents64.count;
        dirp = (struct dirent *)ipc_get_msg_data(ipc_msg);
        dirp_end = (void *)dirp + count;

        while (dirp_end - (void *)dirp >= sizeof(struct dirent)) {
                ret = f_readdir(dp, &finfo);
                if (FR_OK != ret) {
                        return -EINVAL;
                }
                if (finfo.fname[0] == '\0') {
                        break;
                }
                // TODO: this is to imitate tmpfs, but it's really wrong
                dirp->d_ino = finfo.fsize;
                dirp->d_off = sizeof(struct dirent);
                dirp->d_reclen = sizeof(struct dirent);
                dirp->d_type = (finfo.fattrib & AM_DIR) ? DT_DIR : DT_REG;
                strlcpy(dirp->d_name, finfo.fname, D_NAME_MAX_LEN);
                bytes_read += sizeof(struct dirent);
                dirp++;
        }
        return bytes_read;
}

int fat_mount(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int partition = fr->mount.paritition; // use fd field to store "sd
                                              // partition"
        return f_mount(fs, (const TCHAR *)sd, 1, partition);
}

/*
 * TODO: Currently, we use stat to determine if a file or a dir exists,
 * and always return 0 when the file or dir exists.
 */
int fat_faccessat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        FILINFO fno;
        int ret;
        char *path;

        path = fr->faccessat.pathname;

        /* Check path */
        ret = f_stat(path, &fno);
        if (ret != FR_OK) {
                ret = -ENOENT;
                return ret;
        }

        printf("Warning: faccessat always return 0 when the file exists.\n");
        return 0;
}

int fat_fcntl(void *operator, int fd, int cmd, int arg)
{
        return 0;
}

struct fs_server_ops server_ops = {
        .mount = fat_mount,
        .umount = default_server_operation,
        .open = fat_open,
        .read = fat_read,
        .write = fat_write,
        .close = fat_close,
        .creat = default_server_operation,
        .unlink = fat_unlink,
        .mkdir = fat_mkdir,
        .rmdir = fat_rmdir,
        .rename = fat_rename,
        .getdents64 = fat_getdents64,
        .ftruncate = fat_ftruncate,
        .fstatat = fat_fstatat,
        .fstat = fat_fstat,
        .statfs = default_server_operation,
        .fstatfs = default_server_operation,
        .faccessat = fat_faccessat,
        .symlinkat = default_server_operation,
        .readlinkat = default_ssize_t_server_operation,
        .fallocate = default_server_operation,
        .fcntl = fat_fcntl,
        .fmap_get_page_addr = default_fmap_get_page_addr,
};
