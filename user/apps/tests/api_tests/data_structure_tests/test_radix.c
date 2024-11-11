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

#include <chcore/container/radix.h>
#include <stdio.h>
#include <assert.h>

int main()
{
        struct radix radix;
        int ret = 1;
        init_radix(&radix);
        printf("init_radix success.\n");
        ret = radix_add(&radix, 1, (void*)1);
        assert(!ret);
        ret = radix_add(&radix, 3, (void*)3);
        assert(!ret);
        ret = radix_add(&radix, 7, (void*)7);
        assert(!ret);
        printf("radix_add success.\n");
        void *value = radix_get(&radix, 3);
        assert(value);
        printf("Get value %ld\n", (unsigned long)value);
        ret = radix_del(&radix, 1, 0);
        assert(!ret);
        printf("radix_del success.\n");
        printf("Test finish!\n");
        return 0;
}