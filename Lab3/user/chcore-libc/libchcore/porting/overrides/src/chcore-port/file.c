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

#include "limits.h"
#include <stddef.h>
#include <syscall_arch.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <chcore-internal/fs_defs.h>
#include <chcore-internal/lwip_defs.h>
#include <chcore/ipc.h>
#include <chcore-internal/procmgr_defs.h>
#include <pthread.h>

#include "fd.h"
#include "fs_client_defs.h"

#define debug(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)
#define warn_once(fmt, ...)               \
        do {                              \
                static int __warned = 0;  \
                if (__warned)             \
                        break;            \
                __warned = 1;             \
                warn(fmt, ##__VA_ARGS__); \
        } while (0)

/* helper functions */
static inline bool fd_not_exists(int fd)
{
        if (fd < MIN_FD || fd >= MAX_FD || fd_dic[fd] == NULL)
                return true;
        return false;
}

static inline bool fd_not_reserve(int fd)
{
        return fd != AT_FDCWD && fd != AT_FDROOT;
}

static inline int check_path_len(const char *path, int buffer_len)
{
        /* NOTE: max_len of full_path is FS_REQ_PATH_LEN */
        if (strlen(path) > buffer_len) {
                printf("Too long path with path_len=%d\n", strlen(path));
                return -1;
        }
        return 0;
}

static inline bool access_ok(const char *s)
{
        /* TODO: Work around for EFAULT behavior */
        return (s == NULL) ? false : true;
}

/*
 * If `path` begins with '/', ignore dirfd,
 * else concat dirfd corresponding path with `path`
 *
 * Return NULL if error occurs,
 * 	else ALLOCATE buffer for `full_path` and return.
 *
 * NOTE: Don't forget to free return buffer.
 */
static int generate_full_path(int dirfd, const char *path, char **full_path_out)
{
        char *full_path;

        /* Generate full_path */
        if (!path) {
                /* path is NULL */
                if (fd_not_exists(dirfd) && fd_not_reserve(dirfd)) {
                        *full_path_out = NULL;
                        return -EINVAL;
                }
                full_path = strdup(path_from_fd(dirfd));
        } else if (path[0] == '/') {
                /* Absolute Path */
                full_path = strdup(path);
        } else {
                /* Relative Path */
                if (fd_not_exists(dirfd) && fd_not_reserve(dirfd)) {
                        *full_path_out = NULL;
                        return -EINVAL;
                }
                full_path = path_join(path_from_fd(dirfd), path);
        }

        /* Validate full_path */
        if (full_path == NULL) {
                return -ENOMEM;
        }
        if (check_path_len(full_path, FS_REQ_PATH_LEN) != 0) {
                free(full_path);
                *full_path_out = NULL;
                return -ENAMETOOLONG;
        }

        *full_path_out = full_path;
        return 0;
}

/*
 * [IN] full_path: global full path for parsing
 * [OUT] mount_id: Return from FSM. `mount_id` is an identifier for mount_info
 * [OUT] server_path: is the path sending to fs_server,
 * 		removing the prefix of mount_point from `full_path`
 * return: 0 for success, -1 for some error
 */
static inline int parse_full_path(char *full_path, int *mount_id,
                                  char *server_path)
{
        ipc_msg_t *ipc_msg;
        struct fsm_request *fsm_req;
        int ret = 0;

        ipc_msg = ipc_create_msg(fsm_ipc_struct, sizeof(*fsm_req));
        if (ipc_msg == NULL) {
                return -1;
        }
        fsm_req = fsm_parse_path_forward(ipc_msg, full_path);
        if (fsm_req == NULL) {
                ret = -1;
                goto out;
        }
        *mount_id = fsm_req->mount_id;
        if (pathcpy(server_path,
                    FS_REQ_PATH_BUF_LEN,
                    full_path + fsm_req->mount_path_len,
                    strlen(full_path + fsm_req->mount_path_len))
            != 0) {
                ret = -1;
                goto out;
        }

out:
        ipc_destroy_msg(ipc_msg);
        return ret;
}

/* file operations */

int chcore_chdir(const char *path)
{
        /* FIXME: need to check if path exists */
        if (!path) {
                return -EBADF;
        }

        if (path[0] != '/') {
                /* Relative Path */
                path = path_join(cwd_path, path);
        }
        if (pathcpy(cwd_path, MAX_CWD_BUF_LEN, path, strlen(path)) != 0)
                return -EBADF;
        cwd_len = strlen(path);

        return 0;
}

int chcore_fchdir(int fd)
{
        struct fd_record_extension *fd_ext;
        int new_cwd_len;

        if (fd_not_exists(fd))
                return -EBADF;

        fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;

        if (!fd_ext->path)
                return -EBADF;

        new_cwd_len = strlen(fd_ext->path);
        /* if pathcpy failed, the cwd_path will remain the same */
        if (pathcpy(cwd_path, MAX_CWD_BUF_LEN, fd_ext->path, new_cwd_len) != 0)
                return -EBADF;

        /* pathcpy succeed, change cwd_len */
        cwd_len = new_cwd_len;

        return 0;
}

char *chcore_getcwd(char *buf, size_t size)
{
        if (pathcpy(buf, size, cwd_path, cwd_len) != 0)
                return NULL;
        return buf;
}

int chcore_ftruncate(int fd, off_t length)
{
        ipc_msg_t *ipc_msg = 0;
        ipc_struct_t *_fs_ipc_struct;
        struct fd_record_extension *fd_ext;
        struct fs_request *fr_ptr;
        int ret = 0;

        if (fd_not_exists(fd))
                return -EBADF;

        fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;

        _fs_ipc_struct = get_ipc_struct_by_mount_id(fd_ext->mount_id);
        ipc_msg = ipc_create_msg(_fs_ipc_struct, sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_FTRUNCATE;
        fr_ptr->ftruncate.fd = fd;
        fr_ptr->ftruncate.length = length;

        ret = ipc_call(_fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

/**
 * In ChCore, we hijack the syscall function so that many
 * syscalls are forwarded to userspace wrappers like chcore_lseek
 * which implicitly exploits IPC to implement the semantics
 * of hijacked syscall.
 *
 * Note that this is a pure userspace process, which doesn't
 * cross ABI boundary or has nothing to do with ABI, so it's
 * safe for chcore_lseek to return a 64bit off_t on any architectures.
 */
off_t chcore_lseek(int fd, off_t offset, int whence)
{
        ipc_msg_t *ipc_msg = 0;
        ipc_struct_t *_fs_ipc_struct;
        struct fd_record_extension *fd_ext;
        struct fs_request *fr_ptr;
        off_t ret = 0;

        if (fd_not_exists(fd))
                return -EBADF;

        if (fd_dic[fd]->type != FD_TYPE_FILE)
                return -ESPIPE;
        fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;

        _fs_ipc_struct = get_ipc_struct_by_mount_id(fd_ext->mount_id);
        ipc_msg = ipc_create_msg(_fs_ipc_struct, sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_LSEEK;
        fr_ptr->lseek.fd = fd;
        fr_ptr->lseek.offset = offset;
        fr_ptr->lseek.whence = whence;

        ret = ipc_call(_fs_ipc_struct, ipc_msg);
        if (ret == 0) {
                /**
                 * ret == 0 indicates a successful lseek operation,
                 * and we retrieve real offset from fr_ptr(set by server)
                 * to keep the semantic of lseek(2)
                 */
                ret = fr_ptr->lseek.ret;
        }
        ipc_destroy_msg(ipc_msg);

        return ret;
}

int chcore_mkdirat(int dirfd, const char *pathname, mode_t mode)
{
        ipc_msg_t *ipc_msg;
        ipc_struct_t *mounted_fs_ipc_struct;
        struct fs_request *fr_ptr;
        int ret = 0;
        int mount_id;
        char *full_path;
        char server_path[FS_REQ_PATH_BUF_LEN];

        /* Check arguments */
        if (!access_ok(pathname))
                return -EFAULT;

        /* Prepare full_path for IPC arguments, don't forget free(full_path) */
        ret = generate_full_path(dirfd, pathname, &full_path);
        if (ret)
                return ret;

        /* Send IPC to FSM and parse full_path */
        if (parse_full_path(full_path, &mount_id, server_path) != 0) {
                free(full_path);
                return -EINVAL;
        }

        /* Send IPC to fs_server */
        mounted_fs_ipc_struct = get_ipc_struct_by_mount_id(mount_id);
        ipc_msg = ipc_create_msg(mounted_fs_ipc_struct,
                                 sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_MKDIR;
        if (pathcpy(fr_ptr->mkdir.pathname,
                    FS_REQ_PATH_BUF_LEN,
                    server_path,
                    strlen(server_path))
            != 0) {
                ipc_destroy_msg(ipc_msg);
                free(full_path);
                return -EBADF;
        }
        fr_ptr->mkdir.mode = mode;
        ret = ipc_call(mounted_fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        free(full_path);
        return ret;
}

int chcore_unlinkat(int dirfd, const char *pathname, int flags)
{
        ipc_msg_t *ipc_msg;
        ipc_struct_t *mounted_fs_ipc_struct;
        struct fs_request *fr_ptr;
        int ret;
        int mount_id;
        char *full_path;
        char *path;
        char server_path[FS_REQ_PATH_BUF_LEN];

        /* Check argument */
        if (!access_ok(pathname))
                return -EFAULT;

        /* Prepare full_path for IPC arguments, don't forget free(full_path) */
        ret = generate_full_path(dirfd, pathname, &full_path);
        if (ret)
                return ret;

        /* Send IPC to FSM and parse full_path */
        if (parse_full_path(full_path, &mount_id, server_path) != 0) {
                free(full_path);
                return -EINVAL;
        }

        /* Send IPC to fs_server */
        mounted_fs_ipc_struct = get_ipc_struct_by_mount_id(mount_id);
        ipc_msg = ipc_create_msg(mounted_fs_ipc_struct,
                                 sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        if (flags & AT_REMOVEDIR) {
                fr_ptr->req = FS_REQ_RMDIR;
                flags &= (~AT_REMOVEDIR);
                fr_ptr->rmdir.flags = flags;
                path = fr_ptr->rmdir.pathname;
        } else {
                fr_ptr->req = FS_REQ_UNLINK;
                fr_ptr->unlink.flags = flags;
                path = fr_ptr->unlink.pathname;
        }
        if (pathcpy(path, FS_REQ_PATH_BUF_LEN, server_path, strlen(server_path))
            != 0) {
                ipc_destroy_msg(ipc_msg);
                free(full_path);
                return -EBADF;
        }
        ret = ipc_call(mounted_fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        free(full_path);
        return ret;
}

int chcore_symlinkat(const char *target, int newdirfd, const char *linkpath)
{
        ipc_msg_t *ipc_msg;
        ipc_struct_t *mounted_fs_ipc_struct;
        struct fs_request *fr_ptr;
        int ret;
        int mount_id;
        char *full_path;
        char server_path[FS_REQ_PATH_BUF_LEN];

        if (check_path_len(target, FS_REQ_PATH_LEN) != 0)
                return -EINVAL;

        /* Prepare full_path for IPC arguments, don't forget free(full_path) */
        ret = generate_full_path(newdirfd, linkpath, &full_path);
        if (ret)
                return ret;

        /* Send IPC to FSM and parse full_path */
        if (parse_full_path(full_path, &mount_id, server_path) != 0) {
                free(full_path);
                return -EINVAL;
        }

        /* Send IPC to fs_server */
        mounted_fs_ipc_struct = get_ipc_struct_by_mount_id(mount_id);
        ipc_msg = ipc_create_msg(mounted_fs_ipc_struct,
                                 sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_SYMLINKAT;
        if (pathcpy(fr_ptr->symlinkat.linkpath,
                    FS_REQ_PATH_BUF_LEN,
                    full_path,
                    strlen(full_path))
            != 0) {
                ipc_destroy_msg(ipc_msg);
                free(full_path);
                return -EBADF;
        }
        if (pathcpy(fr_ptr->symlinkat.target,
                    FS_REQ_PATH_BUF_LEN,
                    target,
                    strlen(target))
            != 0) {
                ipc_destroy_msg(ipc_msg);
                free(full_path);
                return -EBADF;
        }
        ret = ipc_call(mounted_fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        free(full_path);
        return ret;
}

int chcore_getdents64(int fd, char *buf, int count)
{
        ipc_msg_t *ipc_msg = 0;
        ipc_struct_t *_fs_ipc_struct;
        struct fd_record_extension *fd_ext;
        struct fs_request *fr_ptr;
        int ret = 0, remain = count, cnt;

        if (fd_not_exists(fd))
                return -EBADF;

        BUG_ON(sizeof(struct fs_request) > IPC_SHM_AVAILABLE);

        fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;

        _fs_ipc_struct = get_ipc_struct_by_mount_id(fd_ext->mount_id);
        ipc_msg = ipc_create_msg(_fs_ipc_struct, IPC_SHM_AVAILABLE);
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
        while (remain > 0) {
                fr_ptr->req = FS_REQ_GETDENTS64;
                fr_ptr->getdents64.fd = fd;
                cnt = MIN(remain, IPC_SHM_AVAILABLE);
                fr_ptr->getdents64.count = cnt;
                ret = ipc_call(_fs_ipc_struct, ipc_msg);
                if (ret < 0)
                        goto error;
                memcpy(buf, ipc_get_msg_data(ipc_msg), ret);
                buf += ret;
                remain -= ret;
                if (ret != cnt)
                        break;
        }
        ret = count - remain;
error:
        ipc_destroy_msg(ipc_msg);
        return ret;
}

int chcore_fchmodat(int dirfd, char *path, mode_t mode)
{
        ipc_msg_t *ipc_msg;
        ipc_struct_t *mounted_fs_ipc_struct;
        struct fs_request *fr_ptr;
        int ret = 0;
        int mount_id;
        char *full_path;
        char server_path[FS_REQ_PATH_BUF_LEN];

        /* Check arguments */
        if (!access_ok(path))
                return -EFAULT;

        /* Prepare full_path for IPC arguments, don't forget free(full_path) */
        ret = generate_full_path(dirfd, path, &full_path);
        if (ret)
                return ret;

        /* Send IPC to FSM and parse full_path */
        if (parse_full_path(full_path, &mount_id, server_path) != 0) {
                free(full_path);
                return -EINVAL;
        }

        /* Send IPC to fs_server */
        mounted_fs_ipc_struct = get_ipc_struct_by_mount_id(mount_id);
        ipc_msg = ipc_create_msg(mounted_fs_ipc_struct,
                                 sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_CHMOD;
        if (pathcpy(fr_ptr->chmod.pathname,
                    FS_REQ_PATH_BUF_LEN,
                    server_path,
                    strlen(server_path))
            != 0) {
                ipc_destroy_msg(ipc_msg);
                free(full_path);
                return -EBADF;
        }
        fr_ptr->chmod.mode = mode;
        ret = ipc_call(mounted_fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        free(full_path);
        return ret;
}

int chcore_file_fcntl(int fd, int cmd, int arg)
{
        ipc_msg_t *ipc_msg = 0;
        ipc_struct_t *_fs_ipc_struct;
        struct fd_record_extension *fd_ext, *fd_ext_dup;
        struct fs_request *fr_ptr;
        int new_fd, ret = 0;

        switch (cmd) {
        case F_DUPFD_CLOEXEC:
        case F_DUPFD: {
                new_fd = dup_fd_content(fd, arg);
                if (new_fd < 0) {
                        goto out_fail;
                }
                fd_ext_dup = _new_fd_record_extension();
                if (fd_ext_dup == NULL) {
                        goto out_free_new_fd;
                }
                fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;
                strcpy(fd_ext_dup->path, fd_ext->path);
                fd_ext_dup->mount_id = fd_ext->mount_id;
                fd_dic[new_fd]->private_data = (void *)fd_ext_dup;
                _fs_ipc_struct = get_ipc_struct_by_mount_id(fd_ext->mount_id);
                if (_fs_ipc_struct == NULL)
                        return -EBADF;
                ipc_msg = ipc_create_msg(_fs_ipc_struct,
                                         sizeof(struct fs_request));
                fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
                fr_ptr->req = FS_REQ_FCNTL;
                fr_ptr->fcntl.fcntl_cmd = F_DUPFD;
                fr_ptr->fcntl.fd = fd;
                fr_ptr->fcntl.fcntl_arg = new_fd;
                ret = ipc_call(_fs_ipc_struct, ipc_msg);
                ipc_destroy_msg(ipc_msg);
                return new_fd;
        out_free_new_fd:
                free_fd(new_fd);
        out_fail:
                return ret;
        }
        case F_GETFL:
        case F_SETFL:
                /* FILE fd */
                if (fd_dic[fd]->type == FD_TYPE_FILE) {
                        fd_ext = (struct fd_record_extension *)fd_dic[fd]
                                         ->private_data;
                        if (fd_ext == NULL)
                                return -EBADF;
                        _fs_ipc_struct =
                                get_ipc_struct_by_mount_id(fd_ext->mount_id);
                        if (_fs_ipc_struct == NULL)
                                return -EBADF;
                        ipc_msg = ipc_create_msg(_fs_ipc_struct,
                                                 sizeof(struct fs_request));
                        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

                        fr_ptr->req = FS_REQ_FCNTL;
                        fr_ptr->fcntl.fd = fd;
                        fr_ptr->fcntl.fcntl_cmd = cmd;
                        fr_ptr->fcntl.fcntl_arg = arg;

                        ret = ipc_call(_fs_ipc_struct, ipc_msg);
                        ipc_destroy_msg(ipc_msg);

                        return ret;
                }
                warn("does not support F_SETFL for non-fs files\n");
                return -1;
        default:
                return -EINVAL;
        }
        return -1;
}

int chcore_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz)
{
        ipc_msg_t *ipc_msg = 0;
        ipc_struct_t *mounted_fs_ipc_struct;
        struct fs_request *fr_ptr;
        char *full_path;
        int ret;
        int mount_id;
        char server_path[FS_REQ_PATH_BUF_LEN];

        /* Prepare full_path for IPC arguments, don't forget free(full_path) */
        ret = generate_full_path(dirfd, pathname, &full_path);
        if (ret)
                return ret;

        /* Send IPC to FSM and parse full_path */
        if (parse_full_path(full_path, &mount_id, server_path) != 0) {
                ret = -EINVAL;
                goto out_free_full_path;
        }

        /* Send IPC to fs_server */
        mounted_fs_ipc_struct = get_ipc_struct_by_mount_id(mount_id);
        ipc_msg = ipc_create_msg(mounted_fs_ipc_struct,
                                 sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_READLINKAT;
        if (pathcpy(fr_ptr->readlinkat.pathname,
                    FS_REQ_PATH_BUF_LEN,
                    server_path,
                    strlen(server_path))
            != 0) {
                ret = -EBADF;
                goto out_destroy_msg;
        }
        fr_ptr->readlinkat.bufsiz = bufsiz;
        ret = ipc_call(mounted_fs_ipc_struct, ipc_msg);

        if (ret < 0) {
                goto out_destroy_msg;
        }

        ret = ret > bufsiz ? bufsiz : ret;
        memcpy(buf, ipc_get_msg_data(ipc_msg), ret);

out_destroy_msg:
        ipc_destroy_msg(ipc_msg);
out_free_full_path:
        free(full_path);
        return ret;
}

int chcore_renameat(int olddirfd, const char *oldpath, int newdirfd,
                    const char *newpath)
{
        ipc_msg_t *ipc_msg = 0;
        ipc_struct_t *mounted_fs_ipc_struct;
        struct fs_request *fr_ptr;
        char *old_full_path, *new_full_path;
        int ret, old_mount_id, new_mount_id;
        char old_server_path[FS_REQ_PATH_BUF_LEN];
        char new_server_path[FS_REQ_PATH_BUF_LEN];

        ret = 0;

        /* Check arguments */
        if (!access_ok(oldpath) || !access_ok(newpath))
                return -EFAULT;

        /* Prepare old/new_full_path for IPC arguments */
        ret = generate_full_path(olddirfd, oldpath, &old_full_path);
        if (ret)
                return ret;
        ret = generate_full_path(newdirfd, newpath, &new_full_path);
        if (ret) {
                free(old_full_path);
                return ret;
        }

        /* Send IPC to FSM and parse old_full_path */
        if (parse_full_path(old_full_path, &old_mount_id, old_server_path)
            != 0) {
                free(old_full_path);
                free(new_full_path);
                return -EINVAL;
        }

        if (parse_full_path(new_full_path, &new_mount_id, new_server_path)
            != 0) {
                free(old_full_path);
                free(new_full_path);
                return -EINVAL;
        }

        /* Cross-FS renaming is not supported yet */
        if (old_mount_id != new_mount_id)
                BUG("Does not support cross-FS rename yet.");

        /* Send IPC to fs_server */
        mounted_fs_ipc_struct = get_ipc_struct_by_mount_id(old_mount_id);
        ipc_msg = ipc_create_msg(mounted_fs_ipc_struct,
                                 sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_RENAME;
        pathcpy(fr_ptr->rename.oldpath,
                FS_REQ_PATH_BUF_LEN,
                old_server_path,
                strlen(old_server_path));
        pathcpy(fr_ptr->rename.newpath,
                FS_REQ_PATH_BUF_LEN,
                new_server_path,
                strlen(new_server_path));
        ret = ipc_call(mounted_fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        free(old_full_path);
        free(new_full_path);
        return ret;
}

int chcore_vfs_rename(int oldfd, const char *newpath)
{
        int ret = chcore_renameat(
                AT_FDCWD,
                ((struct fd_record_extension *)fd_dic[oldfd]->private_data)
                        ->path,
                AT_FDCWD,
                newpath);
        return ret;
}

int chcore_faccessat(int dirfd, const char *pathname, int amode, int flags)
{
        ipc_msg_t *ipc_msg = 0;
        ipc_struct_t *mounted_fs_ipc_struct;
        struct fs_request *fr_ptr;
        char *full_path;
        int ret;
        int mount_id;
        char server_path[FS_REQ_PATH_BUF_LEN];

        /* Prepare full_path for IPC arguments, don't forget free(full_path) */
        ret = generate_full_path(dirfd, pathname, &full_path);
        if (ret)
                return ret;

        /* Send IPC to FSM and parse full_path */
        if (parse_full_path(full_path, &mount_id, server_path) != 0) {
                free(full_path);
                return -EINVAL;
        }

        /* Send IPC to fs_server */
        mounted_fs_ipc_struct = get_ipc_struct_by_mount_id(mount_id);
        ipc_msg = ipc_create_msg(mounted_fs_ipc_struct,
                                 sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_FACCESSAT;
        fr_ptr->faccessat.flags = flags;
        fr_ptr->faccessat.amode = amode;
        if (pathcpy(fr_ptr->faccessat.pathname,
                    FS_REQ_PATH_BUF_LEN,
                    server_path,
                    strlen(server_path))
            != 0) {
                ipc_destroy_msg(ipc_msg);
                free(full_path);
                return -EBADF;
        }
        ret = ipc_call(mounted_fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        free(full_path);
        return ret;
}

int chcore_fallocate(int fd, int mode, off_t offset, off_t len)
{
        ipc_msg_t *ipc_msg = 0;
        ipc_struct_t *mounted_fs_ipc_struct;
        struct fd_record_extension *fd_ext;
        struct fs_request *fr_ptr;
        int ret = 0;

        if (fd_not_exists(fd)) {
                return -EBADF;
        }

        fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;

        BUG_ON(fd_ext->mount_id < 0);
        BUG_ON(sizeof(struct fs_request) > IPC_SHM_AVAILABLE); // san check

        mounted_fs_ipc_struct = get_ipc_struct_by_mount_id(fd_ext->mount_id);
        ipc_msg = ipc_create_msg(mounted_fs_ipc_struct,
                                 sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_FALLOCATE;
        fr_ptr->fallocate.fd = fd;
        fr_ptr->fallocate.mode = mode;
        fr_ptr->fallocate.offset = offset;
        fr_ptr->fallocate.len = len;

        ret = ipc_call(mounted_fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

/*
 * This function is never being used, all the functionalities
 * are performed by __xstatxx in syscall_dispatcher.c.
 */
int chcore_statx(int dirfd, const char *pathname, int flags, unsigned int mask,
                 char *buf)
{
        BUG("chcore_statx is never being used!");
        return 0;
}

int chcore_sync(void)
{
        ipc_msg_t *ipc_msg;
        struct fsm_request *fsm_req;
        int ret;

        ipc_msg = ipc_create_msg(fsm_ipc_struct, sizeof(struct fsm_request));
        fsm_req = (struct fsm_request *)ipc_get_msg_data(ipc_msg);

        fsm_req->req = FSM_REQ_SYNC;

        ret = ipc_call(fsm_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

int chcore_fsync(int fd)
{
        ipc_msg_t *ipc_msg;
        ipc_struct_t *_fs_ipc_struct;
        struct fd_record_extension *fd_ext;
        struct fs_request *fr_ptr;
        int ret;

        if (fd_not_exists(fd))
                return -EBADF;

        fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;

        _fs_ipc_struct = get_ipc_struct_by_mount_id(fd_ext->mount_id);
        ipc_msg = ipc_create_msg(_fs_ipc_struct, sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_FSYNC;
        fr_ptr->fsync.fd = fd;

        ret = ipc_call(_fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

int chcore_fdatasync(int fd)
{
        ipc_msg_t *ipc_msg;
        ipc_struct_t *_fs_ipc_struct;
        struct fd_record_extension *fd_ext;
        struct fs_request *fr_ptr;
        int ret;

        if (fd_not_exists(fd))
                return -EBADF;

        fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;

        _fs_ipc_struct = get_ipc_struct_by_mount_id(fd_ext->mount_id);
        ipc_msg = ipc_create_msg(_fs_ipc_struct, sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_FDATASYNC;
        fr_ptr->fdatasync.fd = fd;

        ret = ipc_call(_fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

/* When open we need to alloco fd number first and call to fs server */
int chcore_openat(int dirfd, const char *pathname, int flags, mode_t mode)
{
        struct fd_record_extension *fd_ext;
        struct fs_request *fr_ptr;
        ipc_struct_t *mounted_fs_ipc_struct;
        int ret;
        int mount_id;
        int fd;
        ipc_msg_t *ipc_msg;
        char *full_path;
        char server_path[FS_REQ_PATH_BUF_LEN];

        /*
         * Allocate a fd number first,
         * The fd will be send to fs_server to construct fd->fid mapping
         */
        if ((fd = alloc_fd()) < 0)
                return fd;

        /* Prepare full_path for IPC arguments, don't forget free(full_path) */
        ret = generate_full_path(dirfd, pathname, &full_path);
        if (ret)
                return ret;

        /* Send IPC to FSM and parse full_path */
        if (parse_full_path(full_path, &mount_id, server_path) != 0) {
                free(full_path);
                return -EINVAL;
        }

        /* Send IPC to fs_server */
        mounted_fs_ipc_struct = get_ipc_struct_by_mount_id(mount_id);
        // Fill fd record with IPC information */
        fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;
        fd_ext->mount_id = mount_id;
        if (pathcpy(fd_ext->path, MAX_PATH_BUF_LEN, full_path, strlen(full_path))
            != 0) {
                free(full_path);
                return -EBADF;
        }
        ipc_msg = ipc_create_msg(mounted_fs_ipc_struct,
                                 sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_OPEN;
        fr_ptr->open.new_fd = fd;
        if (pathcpy(fr_ptr->open.pathname,
                    FS_REQ_PATH_BUF_LEN,
                    server_path,
                    strlen(server_path))
            != 0) {
                ipc_destroy_msg(ipc_msg);
                free(full_path);
                return -EBADF;
        }
        fr_ptr->open.flags = flags;
        fr_ptr->open.mode = mode;

        ret = ipc_call(mounted_fs_ipc_struct, ipc_msg);

        if (ret >= 0) {
                fd_dic[fd]->type = FD_TYPE_FILE;
                fd_dic[fd]->fd_op = &file_ops;
                ret = fd; /* Return fd if succeed */
        } else {
                free_fd(fd);
        }

        free(full_path);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

typedef ssize_t (*file_read_ipc_callback)(ipc_struct_t *fs_ipc_struct,
                                          struct ipc_msg *ipc_msg, int fd,
                                          size_t count, off_t offset);

/**
 * Caller should guarantee @count can be casted to ssize_t safely(This is
 * generally true because read(2)/write(2) can only transfer at most 0x7ffff000
 * bytes once).
 */
ssize_t chcore_file_read_core(ipc_struct_t *fs_ipc_struct,
                              file_read_ipc_callback callback, int fd,
                              void *buf, size_t count, off_t initial_offset)
{
        ssize_t ret = 0;
        ssize_t remain = count;
        size_t cnt;
        off_t offset = initial_offset;
        struct ipc_msg *ipc_msg =
                ipc_create_msg(fs_ipc_struct, IPC_SHM_AVAILABLE);

        while (remain > 0) {
                cnt = MIN(remain, IPC_SHM_AVAILABLE);
                ret = callback(fs_ipc_struct, ipc_msg, fd, cnt, offset);
                if (ret > 0) {
                        memcpy(buf, ipc_get_msg_data(ipc_msg), ret);
                } else {
                        break;
                }
                buf = (char *)buf + ret;
                remain -= ret;
                offset += ret;
                if (ret != cnt)
                        break;
        }

        // let -errno propagate to caller code
        if (ret >= 0) {
                ret = (ssize_t)count - remain;
        }

        ipc_destroy_msg(ipc_msg);
        return ret;
}

ssize_t chcore_file_read_cb(ipc_struct_t *fs_ipc_struct,
                            struct ipc_msg *ipc_msg, int fd, size_t count,
                            off_t offset)
{
        struct fs_request *fr_ptr;

        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr_ptr->req = FS_REQ_READ;
        fr_ptr->read.fd = fd;
        fr_ptr->read.count = count;
        /**
         * @offset is ignored because in read IPC offset is managed by server.
         */
        return ipc_call(fs_ipc_struct, ipc_msg);
}

ssize_t chcore_file_pread_cb(ipc_struct_t *fs_ipc_struct,
                             struct ipc_msg *ipc_msg, int fd, size_t count,
                             off_t offset)
{
        struct fs_request *fr_ptr;

        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr_ptr->req = FS_REQ_PREAD;
        fr_ptr->pread.fd = fd;
        fr_ptr->pread.count = count;
        fr_ptr->pread.offset = offset;

        return ipc_call(fs_ipc_struct, ipc_msg);
}

static ssize_t chcore_file_read(int fd, void *buf, size_t count)
{
        ipc_struct_t *_fs_ipc_struct;
        struct fd_record_extension *fd_ext;
        ssize_t ret = 0;
        /**
         * read(2):
         * On Linux, read() (and similar system calls) will transfer at most
         * 0x7ffff000 (2,147,479,552) bytes, returning the number of bytes
         * actually transferred.  (This is true on both 32-bit and 64-bit
         * systems.)
         */
        count = count <= READ_SIZE_MAX ? (ssize_t)count : READ_SIZE_MAX;

        if (fd_not_exists(fd))
                return -EBADF;

        fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;

        BUG_ON(fd_ext->mount_id < 0);
        BUG_ON(sizeof(struct fs_request) > IPC_SHM_AVAILABLE); // san check
        _fs_ipc_struct = get_ipc_struct_by_mount_id(fd_ext->mount_id);

        /**
         * initial_offset could be arbitary value due to read_cb will ignore it.
         */
        ret = chcore_file_read_core(
                _fs_ipc_struct, chcore_file_read_cb, fd, buf, count, 0);
        return ret;
}

static ssize_t chcore_file_pread(int fd, void *buf, size_t count, off_t offset)
{
        ipc_struct_t *_fs_ipc_struct;
        struct fd_record_extension *fd_ext;
        ssize_t ret = 0;
        /**
         * see chcore_file_read
         */
        count = count <= READ_SIZE_MAX ? (ssize_t)count : READ_SIZE_MAX;

        if (fd_not_exists(fd))
                return -EBADF;

        fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;

        BUG_ON(fd_ext->mount_id < 0);
        BUG_ON(sizeof(struct fs_request) > IPC_SHM_AVAILABLE); // san check
        _fs_ipc_struct = get_ipc_struct_by_mount_id(fd_ext->mount_id);

        /**
         * initial_offset could be arbitary value due to read_cb will ignore it.
         */
        ret = chcore_file_read_core(
                _fs_ipc_struct, chcore_file_pread_cb, fd, buf, count, offset);
        return ret;
}

typedef ssize_t (*file_write_ipc_callback)(ipc_struct_t *fs_ipc_struct,
                                           struct ipc_msg *ipc_msg, int fd,
                                           void *buf, size_t count,
                                           off_t offset);

/**
 * Caller should guarantee @count can be casted to ssize_t safely(This is
 * generally true because read(2)/write(2) can only transfer at most 0x7ffff000
 * bytes once).
 */
ssize_t chcore_file_write_core(ipc_struct_t *fs_ipc_struct,
                               file_write_ipc_callback callback, int fd,
                               void *buf, size_t count, off_t initial_offset)
{
        ssize_t ret = 0;
        ssize_t remain = count;
        off_t offset = initial_offset;
        size_t cnt;
        struct ipc_msg *ipc_msg =
                ipc_create_msg(fs_ipc_struct, IPC_SHM_AVAILABLE);

        while (remain > 0) {
                cnt = MIN(remain, FS_BUF_SIZE);
                ret = callback(fs_ipc_struct, ipc_msg, fd, buf, cnt, offset);
                buf = (char *)buf + ret;
                remain -= ret;
                offset += ret;
                if (ret != cnt)
                        break;
        }

        // let -errno propagate to caller code
        if (ret >= 0) {
                ret = (ssize_t)count - remain;
        }

        ipc_destroy_msg(ipc_msg);

        return ret;
}

ssize_t chcore_file_write_cb(ipc_struct_t *fs_ipc_struct,
                             struct ipc_msg *ipc_msg, int fd, void *buf,
                             size_t count, off_t offset)
{
        struct fs_request *fr_ptr;

        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr_ptr->req = FS_REQ_WRITE;
        fr_ptr->write.fd = fd;
        fr_ptr->write.count = count;
        // This offset indicates write position of ipc data, not @offset
        // @offset is ignored due to file offset is managed by server in write
        // operation
        ipc_set_msg_data(ipc_msg, buf, sizeof(struct fs_request), count);

        return ipc_call(fs_ipc_struct, ipc_msg);
}

ssize_t chcore_file_pwrite_cb(ipc_struct_t *fs_ipc_struct,
                              struct ipc_msg *ipc_msg, int fd, void *buf,
                              size_t count, off_t offset)
{
        struct fs_request *fr_ptr;

        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr_ptr->req = FS_REQ_PWRITE;
        fr_ptr->pwrite.fd = fd;
        fr_ptr->pwrite.count = count;
        fr_ptr->pwrite.offset = offset;
        // This offset indicates write position of ipc data, not @offset
        ipc_set_msg_data(ipc_msg, buf, sizeof(struct fs_request), count);

        return ipc_call(fs_ipc_struct, ipc_msg);
}

static ssize_t chcore_file_write(int fd, void *buf, size_t count)
{
        ipc_struct_t *_fs_ipc_struct;
        struct fd_record_extension *fd_ext;
        /**
         * see chcore_file_read
         */
        count = count <= READ_SIZE_MAX ? (ssize_t)count : READ_SIZE_MAX;

        if (fd_not_exists(fd))
                return -EBADF;

        fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;

        BUG_ON(fd_ext->mount_id < 0);
        BUG_ON(sizeof(struct fs_request) > IPC_SHM_AVAILABLE); // san check
        _fs_ipc_struct = get_ipc_struct_by_mount_id(fd_ext->mount_id);

        /**
         * initial_offset could be arbitary value due to write_cb will ignore
         * it.
         */
        return chcore_file_write_core(
                _fs_ipc_struct, chcore_file_write_cb, fd, buf, count, 0);
}

static ssize_t chcore_file_pwrite(int fd, void *buf, size_t count, off_t offset)
{
        ipc_struct_t *_fs_ipc_struct;
        struct fd_record_extension *fd_ext;
        /**
         * see chcore_file_read
         */
        count = count <= READ_SIZE_MAX ? (ssize_t)count : READ_SIZE_MAX;

        if (fd_not_exists(fd))
                return -EBADF;

        fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;

        BUG_ON(fd_ext->mount_id < 0);
        BUG_ON(sizeof(struct fs_request) > IPC_SHM_AVAILABLE); // san check
        _fs_ipc_struct = get_ipc_struct_by_mount_id(fd_ext->mount_id);

        /**
         * initial_offset could be arbitary value due to write_cb will ignore
         * it.
         */
        return chcore_file_write_core(
                _fs_ipc_struct, chcore_file_pwrite_cb, fd, buf, count, offset);
}

static int chcore_file_close(int fd)
{
        ipc_msg_t *ipc_msg;
        ipc_struct_t *_fs_ipc_struct;
        struct fd_record_extension *fd_ext;
        struct fs_request *fr_ptr;
        int ret;

        if (fd_not_exists(fd))
                return -EBADF;

        fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;

        _fs_ipc_struct = get_ipc_struct_by_mount_id(fd_ext->mount_id);
        ipc_msg = ipc_create_msg(_fs_ipc_struct, sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_CLOSE;
        fr_ptr->close.fd = fd;

        ret = ipc_call(_fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        if (ret == 0 || ret == -EBADF) {
                if (fd_dic[fd]->private_data)
                        free(fd_dic[fd]->private_data);
                free_fd(fd);
        }

        return ret;
}

static int chcore_file_ioctl(int fd, unsigned long request, void *arg)
{
        if (request == TIOCGWINSZ)
                return -EBADF;

        WARN("FILE not support ioctl!\n");
        return 0;
}

int chcore_mount(const char *special, const char *dir, const char *fstype,
                 unsigned long flags, const void *data)
{
        ipc_msg_t *ipc_msg;
        struct fsm_request *fsm_req;
        int ret;

        /* Bind 'fs_cap' and 'mount_point' in FSM */
        ipc_msg = ipc_create_msg_with_cap(
                fsm_ipc_struct, sizeof(struct fsm_request), 1);
        fsm_req = (struct fsm_request *)ipc_get_msg_data(ipc_msg);

        fsm_req->req = FSM_REQ_MOUNT;

        /* fsm_req->path = special (device_name) */
        if (pathcpy(fsm_req->path, FS_REQ_PATH_BUF_LEN, special, strlen(special))
            != 0) {
                ipc_destroy_msg(ipc_msg);
                return -EBADF;
        }
        fsm_req->path_len = strlen(special);

        /* fsm_req->mount_path = dir (mount_point) */
        if (pathcpy(fsm_req->mount_path, FS_REQ_PATH_BUF_LEN, dir, strlen(dir))
            != 0) {
                ipc_destroy_msg(ipc_msg);
                return -EBADF;
        }
        fsm_req->mount_path_len = strlen(dir);

        ret = ipc_call(fsm_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

/*
 * @args: special is device name, like 'sda1'...
 * the 'umount' syscall will call this function to send IPC to FSM
 */
int chcore_umount(const char *special)
{
        ipc_msg_t *ipc_msg;
        struct fsm_request *fsm_req;
        int ret;

        ipc_msg = ipc_create_msg_with_cap(
                fsm_ipc_struct, sizeof(struct fsm_request), 1);
        fsm_req = (struct fsm_request *)ipc_get_msg_data(ipc_msg);

        fsm_req->req = FSM_REQ_UMOUNT;

        /* fs_req_data->path = special (device_name) */
        if (pathcpy(fsm_req->path, FS_REQ_PATH_BUF_LEN, special, strlen(special))
            != 0) {
                ipc_destroy_msg(ipc_msg);
                return -EBADF;
        }
        fsm_req->path_len = strlen(special);

        ret = ipc_call(fsm_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

/**
 * Generic function to request stat information.
 * `req` controls which input parameters are used.
 * `statbuf` is always the output (struct stat / struct statfs).
 * Requests using this function include SYS_fstat, SYS_fstatat,
 * SYS_statfs, SYS_fstatfs.
 */
int __xstatxx(int req, int fd, const char *path, int flags, void *statbuf,
              size_t bufsize)
{
        ipc_msg_t *ipc_msg;
        struct fs_request *fr_ptr;
        ipc_struct_t *mounted_fs_ipc_struct;
        char *full_path;
        int ret, mount_id;
        char server_path[FS_REQ_PATH_BUF_LEN];

        if (fd >= 0 && fd <= 2) {
                WARN("fake file stat for fd = 0, 1, 2\n");
                memset(statbuf, 0, bufsize);
                return 0;
        }

        /* Prepare full_path for IPC arguments */
        ret = generate_full_path(fd, path, &full_path);
        if (ret)
                return ret;

        /* Send IPC to FSM and parse full_path */
        if (parse_full_path(full_path, &mount_id, server_path) != 0) {
                free(full_path);
                return -EINVAL;
        }

        mounted_fs_ipc_struct = get_ipc_struct_by_mount_id(mount_id);
        ipc_msg = ipc_create_msg(mounted_fs_ipc_struct,
                                 sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        /* There are 4 type of stat req */
        fr_ptr->req = req;

        /*
         * Some xstat requests use only `fd` argument but no `path`,
         * so if there is no `path`(path==NULL), we should retain `fd` field
         */
        if (path) {
                fr_ptr->stat.fd = fr_ptr->stat.dirfd = AT_FDROOT;
                if (pathcpy(fr_ptr->stat.pathname,
                            FS_REQ_PATH_BUF_LEN,
                            server_path,
                            strlen(server_path))
                    != 0) {
                        ipc_destroy_msg(ipc_msg);
                        free(full_path);
                        return -EBADF;
                }
        } else {
                fr_ptr->stat.fd = fr_ptr->stat.dirfd = fd;
                /* `fr_ptr->path` field is not used in this case */
        }
        fr_ptr->stat.flags = flags;

        ret = ipc_call(mounted_fs_ipc_struct, ipc_msg);
        if (ret == 0) {
                /* No error */
                memcpy(statbuf, ipc_get_msg_data(ipc_msg), bufsize);
        }
        ipc_destroy_msg(ipc_msg);
        if (full_path)
                free(full_path);
        return ret;
}

/* FILE */
struct fd_ops file_ops = {
        .read = chcore_file_read,
        .write = chcore_file_write,
        .pread = chcore_file_pread,
        .pwrite = chcore_file_pwrite,
        .close = chcore_file_close,
        .ioctl = chcore_file_ioctl,
        .poll = NULL,
        .fcntl = chcore_file_fcntl,
};
