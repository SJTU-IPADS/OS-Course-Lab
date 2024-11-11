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

#include <chcore/type.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Fixed badge for root process and servers */
#define ROOT_CAP_GROUP_BADGE (1) /* INIT */
#define PROCMGR_BADGE        ROOT_CAP_GROUP_BADGE
#define FSM_BADGE            (2)
#define LWIP_BADGE           (3)
#define TMPFS_BADGE          (4)
#define MIN_FREE_SERVER_BADGE (5)
#define MIN_FREE_DRIVER_BADGE (100)
#define MIN_FREE_APP_BADGE    (200)

/**
 * Fixed pcid for root process and servers,
 * these should be the same to the definition in cap_group.c
 */
#define ROOT_PROCESS_PCID (1)
#define PROCMGR_PCID      (ROOT_PROCESS_PCID)
#define FSM_PCID          (2)
#define LWIP_PCID         (3)
#define TMPFS_PCID        (4)

pid_t chcore_waitpid(pid_t pid, int *status, int options, int d);

#ifdef __cplusplus
}
#endif
