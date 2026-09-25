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

#include <sys/types.h>
#include <chcore/type.h>
#include <chcore/container/list.h>
#include <chcore/memory.h>

#ifdef __cplusplus
extern "C" {
#endif
#define SHM_RDONLY 010000
#define SHM_RND    020000
#define SHM_REMAP  040000
#define SHM_EXEC   0100000

enum SHM_REQ { SHM_REQ_GET, SHM_REQ_AT, SHM_REQ_DT, SHM_REQ_CTL };

struct shm_request {
        enum SHM_REQ req;
        union {
                struct {
                        int key;
                        int flag;
                        size_t size;
                } shm_get;
                struct {
                        int shmid;
                } shm_at;
                struct {
                        int shmid;
                        int cmd;
                } shm_ctl;
                struct {
                        size_t size;
                        int shmid;
                        cap_t shm_cap;
                } result;
        };
};

#ifdef __cplusplus
}
#endif
