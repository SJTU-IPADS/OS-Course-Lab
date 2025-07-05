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

#define TEST_SUITE(lab, name, ...) void lab##_##name(__VA_ARGS__)
TEST_SUITE(lab4, test_sched_enqueue, struct thread *root_thread);
TEST_SUITE(lab4, test_sched_dequeue, void);
TEST_SUITE(lab4, test_scheduler_meta, void);
TEST_SUITE(lab4, test_timer_init, void);
#endif /* KERNEL_TESTS_RUNTIME_TESTS_H */
