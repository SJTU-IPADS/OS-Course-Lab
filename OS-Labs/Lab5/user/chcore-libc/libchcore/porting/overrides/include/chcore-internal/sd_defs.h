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

#ifdef __cplusplus
extern "C" {
#endif

/* TODO: consider where to put this header file. */

enum SD_REQ {
        SD_REQ_READ = 1,
        SD_REQ_WRITE,
        SD_REQ_MAX,
};

struct sd_msg {
        enum SD_REQ req;
        int block_number;
        int op_size;
        char pbuffer[6 * 512];
};

#ifdef __cplusplus
}
#endif
