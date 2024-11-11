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

#ifndef FSM_DEFS_H
#define FSM_DEFS_H

#include <chcore/ipc.h>
#include <chcore/idman.h>

#define TMPFS_INFO_VADDR 0x200000

#define PREFIX         "[fsm]"
#define info(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#define debug(fmt, ...) \
        do {            \
        } while (0)
#define error(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)

#define MAX_FS_NUM          10
#define MAX_MOUNT_POINT_LEN 255
#define MAX_PATH_LEN        511
#define MBR_MAX_PARTS_CNT   4
#define MBR_SIZE            512
#define MAX_PARTS_CNT       (MBR_MAX_PARTS_CNT + 1)

#endif /* FSM_DEFS_H */