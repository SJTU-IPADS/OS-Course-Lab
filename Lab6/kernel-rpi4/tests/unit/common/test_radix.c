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
#include <stdlib.h>
#include <minunit.h>
#include "test_redef.h"

#include <common/macro.h>

#include "radix.h"

#define printk printf

/* Empty stubs */

int lock_init(struct lock *lk)
{
}

void lock(struct lock *lk)
{
}

void unlock(struct lock *lk)
{
}

void kwarn(const char *fmt, ...)
{
        exit(-1);
}

#include "../../../lib/radix.c"

#include "test.h"

/* The max value should be no less than RND_NODES. */
#define LARGE_VALUE_MAX (VALUE_MAX * VALUE_MAX)

MU_TEST(test_radix)
{
        struct radix radix;
        u64 *ordered_numbers;
        struct dummy_data *data;
        u64 i, p;

        srand(RND_SEED);
        ordered_numbers = malloc(sizeof(*ordered_numbers) * RND_NODES);
        p = 0;

        while (p < RND_NODES) {
                for (i = p; i < RND_NODES; ++i)
                        ordered_numbers[i] = (u64)rand_value(LARGE_VALUE_MAX);
                qsort(ordered_numbers, RND_NODES, sizeof(u64), compare_u64);
                for (p = i = 1; i < RND_NODES; ++i)
                        if (ordered_numbers[i] != ordered_numbers[i - 1])
                                ordered_numbers[p++] = ordered_numbers[i];
        }

        init_radix(&radix);

        /* TEST 1 */
        /* Add 1:1 mapping to the radix. */
        for (i = 0; i < RND_NODES; ++i)
                radix_add(&radix, i, (void *)(s64)(i + 1));

        /* Check simple insertions. */
        for (i = 0; i < RND_NODES; ++i)
                mu_check(radix_get(&radix, i) == (void *)(s64)(i + 1));

        /* Delete first half elements. */
        for (i = 0; i < RND_NODES / 2; ++i)
                radix_del(&radix, i);

        /* Check both deleted and not-deleted elements. */
        for (i = 0; i < RND_NODES; ++i) {
                if (i < RND_NODES / 2)
                        mu_check(radix_get(&radix, i) == NULL);
                else
                        mu_check(radix_get(&radix, i) == (void *)(s64)(i + 1));
        }

        /* Remove all elements. */
        for (i = RND_NODES / 2; i < RND_NODES; ++i)
                radix_del(&radix, i);

        /* Check again, all elements should be deleted. */
        for (i = 0; i < RND_NODES; ++i) {
                mu_check(radix_get(&radix, i) == NULL);
        }

        /* TEST 2 */
        data = malloc(sizeof(*data) * RND_NODES);

        /* Generate and Insert nodes to the radix. */
        for (i = 0; i < RND_NODES; ++i) {
                data[i].index = i;
                data[i].key = ordered_numbers[i];
                data[i].value = 1;
                radix_add(&radix, data[i].key, &data[i]);
                data[i].deleted = false;
        }

        /* Delete some elements randomly. */
        for (i = 0; i < RND_NODES; ++i) {
                if (rand_value(100) & 0x1) {
                        radix_del(&radix, data[i].key);
                        data[i].deleted = true;
                }
        }

        /* Check both deleted and non-deleted elements. */
        for (i = 0; i < RND_NODES; ++i) {
                void *ptr = radix_get(&radix, data[i].key);
                if (data[i].deleted)
                        mu_check(ptr == NULL);
                else
                        mu_check(ptr == &data[i]);
        }

        /* Delete the rest elements. */
        for (i = 0; i < RND_NODES; ++i) {
                if (!data[i].deleted) {
                        radix_del(&radix, data[i].key);
                        data[i].deleted = true;
                }
        }

        /* Delete all elements to be deleted. */
        for (i = 0; i < RND_NODES; ++i)
                mu_check(radix_get(&radix, data[i].key) == NULL);

        free(data);
}

MU_TEST_SUITE(test_suite)
{
        MU_RUN_TEST(test_radix);
}

int main(int argc, char *argv[])
{
        MU_RUN_SUITE(test_suite);
        MU_REPORT();
        return minunit_status;
}
