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

#ifndef KERNEL_TESTS_RUNTIME_TESTS_H
#define KERNEL_TESTS_RUNTIME_TESTS_H

void tst_mutex(void);
void tst_rwlock(void);
void tst_malloc(void);
void tst_sched(void);
#ifdef PLAT_RASPI4
void tst_multi_buddy(void);
#endif

#endif /* KERNEL_TESTS_RUNTIME_TESTS_H */
