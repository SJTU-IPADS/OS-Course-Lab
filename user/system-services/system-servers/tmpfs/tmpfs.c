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

#include "chcore/error.h"
#include "chcore/ipc.h"
#include "fcntl.h"
#include "fs_vnode.h"
#include "pthread.h"
#include "sys/stat.h"
#include <chcore/container/radix.h>
#include <fs_wrapper_defs.h>
#include <chcore/falloc.h>
#include "defs.h"
#include "namei.h"
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

int oflags_to_perm(unsigned int oflags)
{
        int ret = 0;
        unsigned int access_mode;

        access_mode = oflags & O_ACCMODE;

        switch (access_mode) {
        case O_RDONLY:
                ret = TMPFS_READ;
                break;
        case O_WRONLY:
                ret = TMPFS_WRITE;
                break;
        case O_RDWR:
                ret = TMPFS_READ | TMPFS_WRITE;
                break;
        case O_EXEC:
                ret = TMPFS_EXECUTE;
                break;
        }

        return ret;
}

/*
 * This file contains interfaces that fs_base will invoke
 */

/* File operations */

/*
 * POSIX says:
 * The creat() function shall behave as if it is implemented as follows:
 *
 * int creat(const char *path, mode_t mode)
 * {
 *      return open(path, O_WRONLY|O_CREAT|O_TRUNC, mode);
 * }
 *
 * however, this should be done at the fs_base level,
 * when dealing with creat() syscall
 * we still implement a separate creat() function, and try to
 * obey POSIX in tmpfs
 */
int tmpfs_creat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int err;
        struct nameidata nd;
        struct dentry *dentry;
        struct open_flags open_flags;

        char *pathname = fr->creat.pathname;

        open_flags.mode = fr->creat.mode;
        open_flags.o_flags = O_CREAT | O_WRONLY;

        err = path_openat(&nd, pathname, &open_flags, 0, &dentry);
        if (err) {
                goto error;
        }

        /* dentry->inode must be a regular file at this point */
        dentry->inode->f_ops->truncate(dentry->inode, 0);

error:
        return err;
}

int tmpfs_open(char *path, int o_flags, int mode, ino_t *vnode_id,
               off_t *vnode_size, int *vnode_type, void **vnode_private)
{
        /*
         * For monitoring memory usage in tmpfs
         *
         * The current naive implementation use open() to
         * trigger memory usage collecting and show usage info.
         * Details can be found in defs.h
         */
        if (!strcmp(path, "/tmpfs_mem_usage")) {
#if DEBUG_MEM_USAGE
                tmpfs_get_mem_usage();
#endif
                return -ENOENT; /* this should fail */
        }

        int err;
        struct nameidata nd;
        struct dentry *dentry;
        struct open_flags open_flags;

        open_flags.mode = mode;
        open_flags.o_flags = o_flags;
        open_flags.created = 0;

        err = path_openat(&nd, path, &open_flags, 0, &dentry);

        if (!err) {
                struct inode *inode = dentry->inode;
                int perm_desired = oflags_to_perm(open_flags.o_flags);

                /*
                 * if the file is newly created,
                 * we don't check its access mode
                 */
                if (!open_flags.created
                    && !inode->base_ops->check_permission(
                            inode, perm_desired, TMPFS_OWNER)) {
                        err = -EACCES;
                        goto out;
                }

                *vnode_id = (ino_t)(uintptr_t)inode;
                *vnode_size = inode->size;
                *vnode_private = inode;

                /* FIXME: vnode type can not be symlink? */
                switch (inode->type) {
                case FS_REG:
                        *vnode_type = FS_NODE_REG;
                        break;
                case FS_DIR:
                        *vnode_type = FS_NODE_DIR;
                        break;
                default:
                        warn("%s: Unsupported vnode type\n", __func__);
                        break;
                }

                inode->base_ops->open(inode);
        }

out:
        return err;
}

int tmpfs_close(void *operator, bool is_dir /* not handled */, bool do_close)
{
        struct inode *inode = (struct inode *)operator;
        if (do_close) {
                inode->base_ops->close(inode);
        }

        return 0;
}

int tmpfs_chmod(char *pathname, mode_t mode)
{
        int err;
        struct nameidata nd;
        struct dentry *dentry;
        struct inode *inode;

        err = path_lookupat(&nd, pathname, 0, &dentry);

        if (err) {
                return err;
        }

        inode = dentry->inode;
        inode->base_ops->chmod(inode, mode);

        return 0;
}

ssize_t tmpfs_read(void *operator, off_t offset, size_t size, char *buf)
{
        struct inode *inode = (struct inode *)operator;
        BUG_ON(inode->type != FS_REG);

        return inode->f_ops->read(inode, buf, size, offset);
}

ssize_t tmpfs_write(void *operator, off_t offset, size_t size, const char *buf)
{
        struct inode *inode = (struct inode *)operator;
        BUG_ON(inode->type != FS_REG);
        return inode->f_ops->write(inode, buf, size, offset);
}

int tmpfs_ftruncate(void *operator, off_t len)
{
        struct inode *inode = (struct inode *)operator;

        if (inode->type == FS_DIR)
                return -EISDIR;

        if (inode->type != FS_REG)
                return -EINVAL;

        return inode->f_ops->truncate(inode, len);
}

int tmpfs_fallocate(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int fd = fr->fallocate.fd;
        mode_t mode = fr->fallocate.mode;
        off_t offset = fr->fallocate.offset;
        off_t len = fr->fallocate.len;
        int keep_size, ret;
        struct inode *inode;
        struct fs_vnode *vnode;

        vnode = server_entrys[fd]->vnode;
        inode = (struct inode *)vnode->private;

        if (offset < 0 || len <= 0)
                return -EINVAL;

        if (inode->type != FS_REG) {
                return -EBADF;
        }

        pthread_rwlock_wrlock(&vnode->rwlock);

        /* return error if mode is not supported */
        if (mode
            & ~(FALLOC_FL_KEEP_SIZE | FALLOC_FL_PUNCH_HOLE
                | FALLOC_FL_COLLAPSE_RANGE | FALLOC_FL_ZERO_RANGE
                | FALLOC_FL_INSERT_RANGE)) {
                ret = -EOPNOTSUPP;
                goto out;
        }

        if (mode & FALLOC_FL_PUNCH_HOLE) {
                ret = inode->f_ops->punch_hole(inode, offset, len);
        } else if (mode & FALLOC_FL_COLLAPSE_RANGE) {
                ret = inode->f_ops->collapse_range(inode, offset, len);
        } else if (mode & FALLOC_FL_ZERO_RANGE) {
                ret = inode->f_ops->zero_range(inode, offset, len, mode);
        } else if (mode & FALLOC_FL_INSERT_RANGE) {
                ret = inode->f_ops->insert_range(inode, offset, len);
        } else {
                keep_size = mode & FALLOC_FL_KEEP_SIZE ? 1 : 0;

                ret = inode->f_ops->allocate(inode, offset, len, keep_size);
        }

        /**
         * FALLOC_FL_KEEP_SIZE is handled in handlers above, i.e., if size
         * should be kept, the inode->size won't change, vice versa.
         */
        vnode->size = inode->size;
out:
        pthread_rwlock_unlock(&vnode->rwlock);
        return ret;
}

/* Directory operations */
int tmpfs_mkdir(const char *path, mode_t mode)
{
        int err;
        struct nameidata nd;
        struct dentry *d_parent, *d_new_dir;
        struct inode *i_parent;

        err = path_parentat(&nd, path, 0, &d_parent);
        if (err) {
                goto error;
        }

        /* only slashes */
        if (nd.last.str == NULL) {
                return -EBUSY;
        }

        /* creating the new dir */
        i_parent = d_parent->inode;
        d_new_dir =
                i_parent->d_ops->dirlookup(i_parent, nd.last.str, nd.last.len);
        if (d_new_dir) {
                err = -EEXIST;
                goto error;
        }

        /* NOTE: treat all caller as file owner now */
        if (!i_parent->base_ops->check_permission(
                    i_parent, TMPFS_WRITE, TMPFS_OWNER)) {
                err = -EACCES;
                goto error;
        }

        d_new_dir = i_parent->d_ops->alloc_dentry();
        if (CHCORE_IS_ERR(d_new_dir)) {
                err = CHCORE_PTR_ERR(d_new_dir);
                goto error;
        }

        err = i_parent->d_ops->add_dentry(
                i_parent, d_new_dir, nd.last.str, nd.last.len);
        if (err) {
                goto free_dent;
        }

        err = i_parent->d_ops->mkdir(i_parent, d_new_dir, mode);
        if (err) {
                goto remove_dent;
        }

        free_string(&nd.last);
        return 0;

remove_dent:
        i_parent->d_ops->remove_dentry(i_parent, d_new_dir);
free_dent:
        i_parent->d_ops->free_dentry(d_new_dir);
error:
        free_string(&nd.last);
        return err;
}

int tmpfs_rmdir(const char *path, int flags)
{
        int err;
        struct nameidata nd;
        struct dentry *d_parent, *d_to_remove;
        struct inode *i_parent;

        err = path_parentat(&nd, path, 0, &d_parent);
        if (err) {
                goto error;
        }

        /* only slashes */
        if (nd.last.str == NULL) {
                return -EBUSY;
        }

        i_parent = d_parent->inode;

        d_to_remove =
                i_parent->d_ops->dirlookup(i_parent, nd.last.str, nd.last.len);
        if (!d_to_remove) {
                err = -ENOENT;
                goto error;
        }

        if (d_to_remove->inode->type != FS_DIR) {
                err = -ENOTDIR;
                goto error;
        }

        /* NOTE: treat all caller as file owner now */
        if (!i_parent->base_ops->check_permission(
                    i_parent, TMPFS_WRITE, TMPFS_OWNER)) {
                err = -EACCES;
                goto error;
        }

        err = i_parent->d_ops->rmdir(i_parent, d_to_remove);

error:
        free_string(&nd.last);
        return err;
}

int tmpfs_getdents(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int fd = fr->getdents64.fd;
        unsigned int count = fr->getdents64.count;
        char *buff = ipc_get_msg_data(ipc_msg);

        struct inode *inode = (struct inode *)server_entrys[fd]->vnode->private;
        int ret = 0, read_bytes;
        if (!inode) {
                return -ENOENT;
        }

        if (inode->type != FS_DIR) {
                return -ENOTDIR;
        }

        if (!inode->base_ops->check_permission(inode, TMPFS_READ, TMPFS_OWNER)) {
                return -EACCES;
        }

        ret = inode->d_ops->scan(inode,
                                 server_entrys[fd]->offset,
                                 buff,
                                 buff + count,
                                 &read_bytes);

        server_entrys[fd]->offset += ret;
        ret = read_bytes;

        return ret;
}

/* Symbolic link operations */

int tmpfs_symlinkat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int err;
        struct nameidata nd;
        struct dentry *d_parent, *d_symlink;
        struct inode *i_parent, *i_symlink;
        char *target, *path;

        target = fr->symlinkat.target;
        path = fr->symlinkat.linkpath;

        err = path_parentat(&nd, path, 0, &d_parent);
        if (err) {
                goto error;
        }

        /* only slashes */
        if (nd.last.str == NULL) {
                return -EBUSY;
        }

        i_parent = d_parent->inode;

        d_symlink =
                i_parent->d_ops->dirlookup(i_parent, nd.last.str, nd.last.len);
        if (d_symlink) {
                err = -EEXIST;
                goto error;
        }

        /* the path contains trailing slashes, return ENOTDIR per POSIX */
        if (nd.last.str[nd.last.len]) {
                err = -ENOTDIR;
                goto error;
        }

        if (!i_parent->base_ops->check_permission(
                    i_parent, TMPFS_WRITE, TMPFS_OWNER)) {
                err = -EACCES;
                goto error;
        }

        d_symlink = i_parent->d_ops->alloc_dentry();
        if (CHCORE_IS_ERR(d_symlink)) {
                err = CHCORE_PTR_ERR(d_symlink);
                goto error;
        }

        err = i_parent->d_ops->add_dentry(
                i_parent, d_symlink, nd.last.str, nd.last.len);
        if (err) {
                goto free_dent;
        }

        err = i_parent->d_ops->mknod(i_parent, d_symlink, 0, FS_SYM);
        if (err) {
                goto remove_dent;
        }
        i_symlink = d_symlink->inode;

        ssize_t len = i_symlink->sym_ops->write_link(
                i_symlink, target, strlen(target));
        if (len != strlen(target)) {
                err = (int)len;
                goto free_node;
        }

        free_string(&nd.last);
        return 0;

free_node:
        i_symlink->base_ops->free(i_symlink);
remove_dent:
        i_parent->d_ops->remove_dentry(i_parent, d_symlink);
free_dent:
        i_parent->d_ops->free_dentry(d_symlink);
error:
        free_string(&nd.last);
        return err;
}

ssize_t tmpfs_readlinkat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int err;
        struct nameidata nd;
        struct dentry *d_symlink;
        struct inode *i_symlink;

        const char *path;
        char *buff;
        ssize_t len;

        path = fr->readlinkat.pathname;
        buff = fr->readlinkat.buf;
        len = (ssize_t)fr->readlinkat.bufsiz;

        err = path_lookupat(&nd, path, 0, &d_symlink);
        if (err) {
                return err;
        }

        i_symlink = d_symlink->inode;
        if (i_symlink->type != FS_SYM) {
                return -EINVAL;
        }
        len = i_symlink->sym_ops->read_link(i_symlink, buff, len);

        return len;
}

/* File and directory operations */
int tmpfs_unlink(const char *path, int flags)
{
        int err;
        struct nameidata nd;
        struct dentry *d_parent, *d_unlink;
        struct inode *i_parent;

        if (flags & (~AT_SYMLINK_NOFOLLOW)) {
                return -EINVAL;
        }

        err = path_parentat(&nd, path, 0, &d_parent);
        if (err) {
                goto error;
        }

        /* only slashes */
        if (nd.last.str == NULL) {
                return -EBUSY;
        }

        i_parent = d_parent->inode;

        d_unlink =
                i_parent->d_ops->dirlookup(i_parent, nd.last.str, nd.last.len);
        if (!d_unlink) {
                err = -ENOENT;
                goto error;
        }

        if (d_unlink->inode->type == FS_DIR) {
                err = -EISDIR;
                goto error;
        }

        if (!i_parent->base_ops->check_permission(
                    i_parent, TMPFS_WRITE, TMPFS_OWNER)) {
                err = -EACCES;
                goto error;
        }

        i_parent->d_ops->unlink(i_parent, d_unlink);

error:
        free_string(&nd.last);
        return err;
}

/*
 * Some facts is ensured when tmpfs_rename is called by fs_base:
 *     1. the two paths is not ended with dots
 *     2. oldpath actually exists
 *     3. oldpath is not an ancestor of new
 *     4. newpath has a valid, existed prefix and it's a dir
 *     5. if newpath exists, the two paths' types match
 *     6. if newpath exists, the newpath is removed.
 *
 * NOTE: dependency on fs_base
 * The implementation relies on these facts so it can be simple,
 * but if the implementation of fs_base changed, the implementation
 * here may also need modification
 */
int tmpfs_rename(const char *oldpath, const char *newpath)
{
        int err;
        struct nameidata nd;
        struct dentry *d_old_parent, *d_new_parent, *d_old, *d_new;
        struct inode *i_old_parent, *i_new_parent;

        err = path_parentat(&nd, oldpath, 0, &d_old_parent);
        if (err) {
                error("oldpath should exist!\n");
                goto error;
        }

        /* only slashes */
        if (nd.last.str == NULL) {
                return -EBUSY;
        }

        i_old_parent = d_old_parent->inode;

        if (!i_old_parent->base_ops->check_permission(
                    i_old_parent, TMPFS_WRITE, TMPFS_OWNER)) {
                return -EACCES;
        }

        d_old = i_old_parent->d_ops->dirlookup(
                i_old_parent, nd.last.str, nd.last.len);

        free_string(&nd.last);

        err = path_parentat(&nd, newpath, 0, &d_new_parent);
        if (err) {
                error("newpath prefix should exist!\n");
                goto error;
        }

        /* only slashes */
        if (nd.last.str == NULL) {
                return -EBUSY;
        }

        i_new_parent = d_new_parent->inode;

        if (!i_new_parent->base_ops->check_permission(
                    i_new_parent, TMPFS_WRITE, TMPFS_OWNER)) {
                return -EACCES;
        }

        d_new = i_new_parent->d_ops->alloc_dentry();
        if (CHCORE_IS_ERR(d_new)) {
                err = CHCORE_PTR_ERR(d_new);
                goto error;
        }

        err = i_new_parent->d_ops->add_dentry(
                i_new_parent, d_new, nd.last.str, nd.last.len);
        if (err) {
                goto free_dent;
        }

        i_old_parent->d_ops->rename(i_old_parent, d_old, i_new_parent, d_new);

        free_string(&nd.last);
        return 0;

free_dent:
        i_new_parent->d_ops->free_dentry(d_new);
error:
        free_string(&nd.last);
        return err;
}

/* File metadata operations */

int tmpfs_fstatat(const char *path, struct stat *st, int flags)
{
        int err;
        struct nameidata nd;
        struct dentry *dentry;
        struct inode *inode;

        /*
         * POSIX says we should follow the trailing symlink,
         * except AT_SYMLINK_NOFOLLOW is set in @flags
         */
        err = path_lookupat(&nd,
                            path,
                            flags & AT_SYMLINK_NOFOLLOW ? 0 : ND_FOLLOW,
                            &dentry);
        if (err) {
                return err;
        }

        inode = dentry->inode;
        inode->base_ops->stat(inode, st);
        return 0;
}

int tmpfs_fstat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        struct inode *inode;

        int fd = fr->stat.fd;
        struct stat *stat = &fr->stat.stat;

        BUG_ON(!server_entrys[fd]);
        inode = (struct inode *)server_entrys[fd]->vnode->private;

        inode->base_ops->stat(inode, stat);

        return 0;
}

int tmpfs_faccessat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int err;
        bool allowed = true;
        struct nameidata nd;
        struct dentry *dentry;
        struct inode *inode;

        const char *path = fr->faccessat.pathname;
        int amode = fr->faccessat.amode;
        int flags = fr->faccessat.flags;

        if (flags & (~AT_EACCESS)) {
                return -EINVAL;
        }

        /* look the path up does all the job */
        err = path_lookupat(&nd, path, 0, &dentry);

        if (err || amode & F_OK) {
                return err;
        }

        inode = dentry->inode;

        if (amode & R_OK) {
                allowed &= ((inode->mode >> 6) & TMPFS_READ) == TMPFS_READ;
        }
        if (amode & W_OK) {
                allowed &= ((inode->mode >> 6) & TMPFS_WRITE) == TMPFS_WRITE;
        }
        if (amode & X_OK) {
                allowed &= ((inode->mode >> 6) & TMPFS_EXECUTE)
                           == TMPFS_EXECUTE;
        }

        return allowed ? 0 : -EACCES;
}

/* File system metadata operations */

int tmpfs_statfs(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int err;
        struct nameidata nd;
        struct dentry *dentry;

        /*
         * dirfd is actually guaranteed to be root dir,
         * so we are not using it in tmpfs
         */
        const char *path = fr->stat.pathname;
        struct statfs *stat = &fr->stat.statfs;

        err = path_lookupat(&nd, path, 0, &dentry);
        if (!err) {
                tmpfs_fs_stat(stat);
        }

        return err;
}

int tmpfs_fstatfs(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
        int fd = fr->stat.fd;
        struct statfs *stat = &fr->stat.statfs;

        BUG_ON(!server_entrys[fd]);

        tmpfs_fs_stat(stat);

        return 0;
}

/* Additional functionalities */
int tmpfs_fcntl(void *operator, int fd, int fcntl_cmd, int fcntl_arg)
{
        struct server_entry *entry;
        int ret = 0;

        entry = server_entrys[fd];
        if (entry == NULL)
                return -EBADF;

        switch (fcntl_cmd) {
        case F_GETFL:
                ret = entry->flags;
                break;
        case F_SETFL: {
                // file access mode and the file creation flags
                // should be ignored, per POSIX
                int effective_bits = (~O_ACCMODE & ~O_CLOEXEC & ~O_CREAT
                                      & ~O_DIRECTORY & ~O_EXCL & ~O_NOCTTY
                                      & ~O_NOFOLLOW & ~O_TRUNC & ~O_TTY_INIT);

                entry->flags = (fcntl_arg & effective_bits)
                               | (entry->flags & ~effective_bits);
                break;
        }
        case F_DUPFD:
                break;

        case F_GETOWN:
                /* Fall through */
        case F_SETOWN:
        case F_GETLK:
        case F_SETLK:
        case F_SETLKW:
        default:
                error("unsupported fcntl cmd\n");
                ret = -1;
                break;
        }

        return ret;
}

vaddr_t tmpfs_get_page_addr(void *operator, off_t offset)
{
        struct inode *inode;
        void *page;
        off_t page_no;

        inode = (struct inode *)operator;
        page_no = offset / PAGE_SIZE;
        page = radix_get(&inode->data, page_no);

        /* the case of truncated to a larger size but not yet allocated */
        if (!page && offset < inode->size) {
                page = aligned_alloc(PAGE_SIZE, PAGE_SIZE);
                if (!page) {
                        return -ENOMEM;
                }
#if DEBUG_MEM_USAGE
                tmpfs_record_mem_usage(page, PAGE_SIZE, DATA_PAGE);
#endif

                /* set page to 0 and cause page to be actually mapped */
                memset(page, 0, PAGE_SIZE);
                radix_add(&inode->data, page_no, page);
        }

        return (vaddr_t)page;
}

struct fs_server_ops server_ops = {
        /* Unimplemented */
        .mount = default_server_operation,
        .umount = default_server_operation,

        /* File Operations */
        .creat = tmpfs_creat,
        .open = tmpfs_open,
        .close = tmpfs_close,
        .chmod = tmpfs_chmod,
        .read = tmpfs_read,
        .write = tmpfs_write,
        .ftruncate = tmpfs_ftruncate,
        .fallocate = tmpfs_fallocate,

        /* Directory Operations */
        .mkdir = tmpfs_mkdir,
        .rmdir = tmpfs_rmdir,
        .getdents64 = tmpfs_getdents,

        /* Symbolic Link Operations */
        .symlinkat = tmpfs_symlinkat,
        .readlinkat = tmpfs_readlinkat,

        /* File and Directory Operations */
        .unlink = tmpfs_unlink,
        .rename = tmpfs_rename,

        /* File Metadata Operations */
        .fstatat = tmpfs_fstatat,
        .fstat = tmpfs_fstat,
        .faccessat = tmpfs_faccessat,

        /* File System Metadata Operations */
        .statfs = tmpfs_statfs,
        .fstatfs = tmpfs_fstatfs,

        /* Additional Functionalities */
        .fcntl = tmpfs_fcntl,
        .fmap_get_page_addr = tmpfs_get_page_addr,
};
