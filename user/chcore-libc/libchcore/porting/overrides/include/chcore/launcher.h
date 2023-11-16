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

struct new_process_caps {
        /* The cap of the main thread of the newly created process */
        cap_t mt_cap;
};

int chcore_new_process(int argc, char *argv[]);

int create_process(int argc, char *argv[], struct new_process_caps *caps);

int create_traced_process(int argc, char *__argv[], struct new_process_caps *caps);

#ifdef __cplusplus
}
#endif
