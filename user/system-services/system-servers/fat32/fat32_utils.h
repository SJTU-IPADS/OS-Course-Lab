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

#pragma once

#include <bits/errno.h>
#include <chcore/type.h>
#include <chcore/ipc.h>
#include <chcore/bug.h>
#include <chcore/syscall.h>
#include <pthread.h>
#include <malloc.h>
#include <assert.h>
#include <chcore-internal/fs_defs.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <string.h>

#include <fs_wrapper_defs.h>
#include <chcore-internal/fs_debug.h>
#include "ff.h"
#include "diskio.h"

void free_filfd(int fd);
int find_filfd(int isfile);
int flags_posix2internal(int flags);
void fill_stat(struct stat *statbuf, FILINFO *fno, ino_t inum);
