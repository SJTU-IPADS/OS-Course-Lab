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

#include <stdio.h>
#include <string.h>
#include <object/cap_group.h>
#include <object/thread.h>
#include "test_object.h"

int sys_test_object_set_int(cap_t cap, unsigned long arg)
{
        struct test_object *test_obj;
        test_obj = obj_get(current_cap_group, cap, TYPE_TEST_OBJECT);
        if (!test_obj) {
                return -ECAPBILITY;
        }
        test_obj->test_int = arg;
        printf("set test_int to 0x%lx\n", test_obj->test_int);
        obj_put(test_obj);
        return 0;
}

int sys_test_object_get_int(cap_t cap)
{
        struct test_object *test_obj;
        test_obj = obj_get(current_cap_group, cap, TYPE_TEST_OBJECT);
        if (!test_obj) {
                return -ECAPBILITY;
        }
        printf("get test_int 0x%lx\n", test_obj->test_int);
        obj_put(test_obj);
        return 0;
}
