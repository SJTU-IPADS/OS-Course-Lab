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

#ifndef FSM_H
#define FSM_H

#include <assert.h>
//#include <chcore/cpio.h>
#include <chcore/defs.h>
#include <chcore/error.h>
#include <chcore-internal/fs_defs.h>
#include <chcore/container/list.h>
#include <chcore/ipc.h>
#include <chcore/launcher.h>
#include <chcore/proc.h>
#include <chcore/syscall.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include "defs.h"

/* for debugging */
#include <chcore-internal/fs_debug.h>

int init_fsm(void);
int fsm_mount_fs(const char *path, const char *mount_point);
int fsm_umount_fs(const char *path);

#endif /* FSM_H */
