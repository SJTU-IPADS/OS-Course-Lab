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
#include <chcore-internal/fs_defs.h>
#include <chcore-internal/fs_debug.h>
#include <fs_wrapper_defs.h>
#include <fcntl.h>
#include <pthread.h>
#include <malloc.h>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include "ext4.h"
#include "file_dev.h"
#include <chcore-internal/fs_defs.h>
#include "chcore_ext4_server.h"
#include "chcore_ext4_defs.h"

#define MAX_ENTRY         127
#define CHECK(condition)  check_r(condition, __FILE__, __LINE__)
#define EXT4_PATH_LEN_MAX (FS_REQ_PATH_LEN)
#define EXT4_PATH_BUF_LEN (EXT4_PATH_LEN_MAX + 1)

/* aquired by lwext4 */
extern struct ext4_blockdev *bd;

/* ext4 parition offset using in file_dev */
extern uint64_t partition_lba;

/* concurrency control */
extern pthread_mutex_t ext4_meta_lock;

int init_device(void);
int umount_device(void);
int fillstat(struct stat *st, const char *path);
void check_r(int condition, char *_file, int _line);
int ext4_get_inode_type(const char *path);
int ext4_dir_is_empty(const char *path);
int check_path_prefix_is_dir(const char *path);
int check_path_leaf_is_not_dot(const char *path);
