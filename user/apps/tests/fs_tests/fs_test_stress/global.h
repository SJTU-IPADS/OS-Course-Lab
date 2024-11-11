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

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2000-2003 Silicon Graphics, Inc.
 * All Rights Reserved.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#define _GNU_SOURCE

#include <stdio.h>
#include <limits.h>

#ifdef HAVE_ATTR_ATTRIBUTES_H
#include <attr/attributes.h>
#endif

#ifndef O_DIRECT
#define O_DIRECT 0200000
#endif
#define loff_t     off_t
#define off64_t    off_t
#define stat64     stat
#define lseek64    lseek
#define lstat64    lstat
#define fstat64    fstat
#define readdir64  readdir
#define truncate64 truncate

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#include <sys/types.h>

#include <sys/stat.h>

#include <sys/statvfs.h>

#include <sys/time.h>

#include <sys/ioctl.h>

#include <sys/wait.h>

#include <malloc.h>

#include <dirent.h>

#include <stdlib.h>

#include <unistd.h>

#include <errno.h>

#include <string.h>

#include <fcntl.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_ATTRIBUTES_H
#include <sys/attributes.h>
#endif

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include <assert.h>

#include <signal.h>
#include <stdint.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_SYS_SYSSGI_H
#include <sys/syssgi.h>
#endif

#ifdef HAVE_SYS_UUID_H
#include <sys/uuid.h>
#endif

#ifdef HAVE_SYS_FS_XFS_FSOPS_H
#include <sys/fs/xfs_fsops.h>
#endif

#ifdef HAVE_SYS_FS_XFS_ITABLE_H
#include <sys/fs/xfs_itable.h>
#endif

#ifdef HAVE_BSTRING_H
#include <bstring.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_LINUX_FALLOC_H
#include <linux/falloc.h>

#ifndef FALLOC_FL_KEEP_SIZE
#define FALLOC_FL_KEEP_SIZE 0x01
#endif

#ifndef FALLOC_FL_PUNCH_HOLE
#define FALLOC_FL_PUNCH_HOLE 0x02
#endif

#ifndef FALLOC_FL_NO_HIDE_STALE
#define FALLOC_FL_NO_HIDE_STALE 0x04
#endif

#ifndef FALLOC_FL_COLLAPSE_RANGE
#define FALLOC_FL_COLLAPSE_RANGE 0x08
#endif

#ifndef FALLOC_FL_ZERO_RANGE
#define FALLOC_FL_ZERO_RANGE 0x10
#endif

#ifndef FALLOC_FL_INSERT_RANGE
#define FALLOC_FL_INSERT_RANGE 0x20
#endif

#endif /* HAVE_LINUX_FALLOC_H */

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#endif /* GLOBAL_H */
