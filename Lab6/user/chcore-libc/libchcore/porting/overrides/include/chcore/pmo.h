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

struct pmo_request {
        /* input: args */
        unsigned long size;
        unsigned long type;

        /* output: return value */
        cap_t ret_cap;
};

struct pmo_map_request {
        /* input: args */
        cap_t pmo_cap;
        unsigned long addr;
        unsigned long perm;

        /* output: return value */
        int ret;
};

#ifdef __cplusplus
}
#endif
