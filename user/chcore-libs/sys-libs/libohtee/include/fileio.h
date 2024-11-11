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

#ifndef SYS_FILEIO_H
#define SYS_FILEIO_H

#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>

#define TAFS_MOUNTPOINT "/tafs"

int32_t fileio_init(void);
int32_t open(const char *pathname, int32_t flags, ...);
int32_t close(int32_t fd);
ssize_t read(int32_t fd, void *buf, size_t count);
ssize_t write(int32_t fd, const void *buf, size_t count);
int32_t ftruncate(int32_t fd, int64_t length);
int32_t unlink(const char *pathname);
int32_t vfs_rename(int32_t fd, const char *new_name);
void *vfs_mmap(int32_t fd, uint64_t size, uint64_t off);

#endif
