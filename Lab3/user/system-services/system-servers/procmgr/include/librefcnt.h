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

#ifndef LIBREFCNT_H
#define LIBREFCNT_H

#include <chcore/type.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*obj_deinit_t)(void *obj);

void *obj_alloc(size_t size, obj_deinit_t obj_deinit);
void *obj_get(void *obj);
void obj_put(void *obj);

#ifdef __cplusplus
}
#endif

#endif /* LIBREFCNT_H */