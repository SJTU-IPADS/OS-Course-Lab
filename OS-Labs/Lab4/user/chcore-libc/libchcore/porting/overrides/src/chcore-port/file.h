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

#ifndef CHCORE_PORT_FILE_H
#define CHCORE_PORT_FILE_H

#include <stddef.h>
#include <sys/types.h>

int chcore_chdir(const char *path);
int chcore_fchmodat(int dirfd, char *path, mode_t mode);
int chcore_fchdir(int fd);
long chcore_getcwd(char *buf, size_t size);
int chcore_ftruncate(int fd, off_t length);
off_t chcore_lseek(int fd, off_t offset, int whence);
int chcore_mkdirat(int dirfd, const char *pathname, mode_t mode);
int chcore_unlinkat(int dirfd, const char *pathname, int flags);
int chcore_symlinkat(const char *target, int newdirfd, const char *linkpath);
int chcore_getdents64(int fd, char *buf, int count);
int chcore_fcntl(int fd, int cmd, int arg);
int chcore_readlinkat(int dirfd, const char *pathname, char *buf,
                      size_t bufsiz);
int chcore_renameat(int olddirfd, const char *oldpath, int newdirfd,
                    const char *newpath);
int chcore_vfs_rename(int, const char *);
int chcore_faccessat(int dirfd, const char *pathname, int amode, int flags);
int chcore_fallocate(int fd, int mode, off_t offset, off_t len);
int chcore_statx(int dirfd, const char *pathname, int flags, unsigned int mask,
                 char *buf);
int chcore_sync(void);
int chcore_fsync(int fd);
int chcore_fdatasync(int fd);
int chcore_openat(int dirfd, const char *pathname, int flags, mode_t mode);
int chcore_mount(const char *special, const char *dir, const char *fstype,
                 unsigned long flags, const void *data);
int chcore_umount(const char *special);

#endif /* CHCORE_PORT_FILE_H */
