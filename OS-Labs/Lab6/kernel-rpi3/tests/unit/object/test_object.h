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

#ifndef TEST_OBJECT_H
#define TEST_OBJECT_H

#include <object/object.h>

enum test_object_type { TYPE_TEST_OBJECT = TYPE_NR, TYPE_TEST_NR };

struct test_object {
        unsigned long test_int;
};

int sys_test_object_set_int(cap_t cap, unsigned long arg);
int sys_test_object_get_int(cap_t cap);

#endif /* TEST_OBJECT_H */
