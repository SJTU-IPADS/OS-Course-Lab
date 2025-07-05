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

#ifndef CHANMGR_DEFS_H
#define CHANMGR_DEFS_H

#include <chcore/ipc.h>
#include <sys/types.h>

#include <ipclib.h>

#ifdef __cplusplus
extern "C" {
#endif

enum CHAN_REQ {
        CHAN_REQ_CREATE_CHANNEL = 1,
        CHAN_REQ_REMOVE_CHANNEL,
        CHAN_REQ_HUNT_BY_NAME,
        CHAN_REQ_GET_CH_FROM_PATH,
        CHAN_REQ_GET_CH_FROM_TASKID,
        CHAN_REQ_MAX
};

#define CHAN_REQ_NAME_LEN 512

struct chan_request {
        enum CHAN_REQ req;
        union {
                struct {
                        int ch_num;
                        char name[CHAN_REQ_NAME_LEN];
                        struct reg_items_st reg_items;
                } create_channel;

                struct {
                        uint32_t taskid;
                        int ch_num;
                        char name[CHAN_REQ_NAME_LEN];
                } remove_channel;

                struct {
                        uint32_t taskid;
                        char name[CHAN_REQ_NAME_LEN];
                } hunt_by_name;

                struct {
                        char name[CHAN_REQ_NAME_LEN];
                } get_ch_from_path;

                struct {
                        uint32_t taskid;
                        int ch_num;
                } get_ch_from_taskid;
        };
};

#ifdef __cplusplus
}
#endif

#endif /* CHANMGR_DEFS_H */
