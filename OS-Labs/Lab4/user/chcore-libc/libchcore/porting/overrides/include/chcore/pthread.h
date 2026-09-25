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

#ifndef CHCORE_PTHREAD_H
#define CHCORE_PTHREAD_H

#include <chcore/type.h>

#ifndef __NEED_pthread_t
#define __NEED_pthread_t
#endif

#ifndef __NEED_pthread_attr_t
#define __NEED_pthread_attr_t
#endif

#if __SIZEOF_POINTER__ == 4
#define CHCORE_PTHREAD_DEFAULT_STACK_SIZE (1<<20)
#define CHCORE_PTHREAD_DEFAULT_GUARD_SIZE 4096
#else
#define CHCORE_PTHREAD_DEFAULT_STACK_SIZE (8<<20)
#define CHCORE_PTHREAD_DEFAULT_GUARD_SIZE 8192
#endif

#include <bits/alltypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Chcore pthread_create: return the thread_cap */
cap_t chcore_pthread_create(pthread_t *__restrict, const pthread_attr_t *__restrict, void *(*)(void *), void *__restrict);
cap_t chcore_pthread_create_shadow(pthread_t *__restrict, const pthread_attr_t *__restrict, void *(*)(void *), void *__restrict);
cap_t chcore_pthread_create_register_cb(pthread_t *__restrict, const pthread_attr_t *__restrict, void *(*)(void *), void *__restrict);
#ifdef CHCORE_OPENTRUSTEE
void chcore_pthread_wake_joiner(pthread_t thread);
int chcore_pthread_get_tid(pthread_t thread);
#endif /* CHCORE_OPENTRUSTEE */

#ifdef __cplusplus
}
#endif

#endif