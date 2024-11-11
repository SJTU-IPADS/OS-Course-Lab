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
#include <sys/stat.h>
#include <sys/statfs.h>
#include <string.h>

#include <chcore-internal/fs_defs.h>
#include <chcore-internal/fs_debug.h>
#include <fs_wrapper_defs.h>
#include <fs_vnode.h>
#include "ff.h"
#include "diskio.h"
#include "fat32_utils.h"
#include "fat32_defs.h"

/* Translate the POSIX flags to fat flags */
int flags_posix2internal(int flags)
{
        int mode = FA_READ;
        if (flags & O_CREAT)
                mode |= (FA_WRITE | FA_READ | FA_OPEN_ALWAYS);
        if (flags & O_WRONLY)
                mode |= FA_WRITE;
        if (flags & O_RDWR)
                mode |= (FA_READ | FA_WRITE);
        if (flags & O_APPEND)
                mode |= FA_OPEN_APPEND;
        return mode;
}

void fill_stat(struct stat *statbuf, FILINFO *fno, ino_t inum)
{
        assert(fno);
        statbuf->st_dev = 0;
        statbuf->st_ino = inum;

        if (fno->fattrib & AM_DIR) {
                statbuf->st_mode = S_IFDIR;
        } else {
                statbuf->st_mode = S_IFREG;
        }

        statbuf->st_nlink = 1;
        /* TODO(YSM): uid/gid. */
        statbuf->st_uid = 0;
        statbuf->st_gid = 0;

        statbuf->st_rdev = 0;
        statbuf->st_size = fno->fsize;

        /* TODO(YSM): Use real xtime. */
        statbuf->st_atime = fno->ftime;
        statbuf->st_mtime = fno->ftime;
        statbuf->st_ctime = fno->ftime;

        /* TODO(YSM): Handle the following two. */
        statbuf->st_blksize = 0;
        statbuf->st_blocks = 0;
}