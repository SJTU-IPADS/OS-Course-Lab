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

#ifndef COMMON_SYNC_H
#define COMMON_SYNC_H

#include <arch/sync.h>

#if __SIZEOF_POINTER__ == 4
#define atomic_fetch_sub_long(ptr, val) atomic_fetch_sub_32(ptr, val)
#define atomic_fetch_add_long(ptr, val) atomic_fetch_add_32(ptr, val)
#else
#define atomic_fetch_sub_long(ptr, val) atomic_fetch_sub_64(ptr, val)
#define atomic_fetch_add_long(ptr, val) atomic_fetch_add_64(ptr, val)
#endif

#endif /* COMMON_SYNC_H */