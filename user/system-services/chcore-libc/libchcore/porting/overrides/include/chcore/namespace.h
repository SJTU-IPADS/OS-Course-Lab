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

#ifndef CHCORE_NAMESPACE_H
#define CHCORE_NAMESPACE_H

#include <chcore/file_namespace.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_NS (0x1)

struct proc_ns {
        struct user_file_namespace fs_ns; 
};

#define proc_ns_fs(ns) (&(ns)->fs_ns)

int proc_ns_init(struct proc_ns *ns);

#ifdef __cplusplus
}
#endif

#endif /* CHCORE_NAMESPACE_H */
