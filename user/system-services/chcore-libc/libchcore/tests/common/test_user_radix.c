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

//#include <common/macro.h>

#include <chcore/container/radix.h>

//#define RND_NODES (10000)
#define RND_NODES (10000 * 100)
//#define RND_NODES (3)
#define RND_SEED (1024)
#define VALUE_MAX (123456LLU)
static inline u64 rand_value(u64 max)
{
	return (rand() % max) * (rand() % max) % max;
}

struct dummy_data {
	u64 key;
	u64 value;
	int index;
	bool deleted;
};

int compare_u64(const void *a, const void *b)
{
	u64 ua = *(u64*)a;
	u64 ub = *(u64*)b;

	if (ua > ub) return 1;
	if (ua == ub) return 0;
	if (ua < ub) return -1;
}

/* The max value should be no less than RND_NODES. */
#define LARGE_VALUE_MAX (VALUE_MAX * VALUE_MAX)


struct radix_scan_cb_args {
	u64 next;
	u64 step;
};

int check_scan(void *value, void *__args)
{
	struct radix_scan_cb_args *args = __args;

	if ((u64)value != args->next)
		return -1;

	args->next += args->step;

	return 0;
}

MU_TEST(test_radix)
{
	struct radix radix;
	u64 *ordered_numbers;
	struct dummy_data *data;
	int err;
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
		radix_add(&radix, i, (void *)(s64)(i+1));

	/* Check simple insertions. */
	for (i = 0; i < RND_NODES; ++i)
		mu_check(radix_get(&radix, i) == (void *)(s64)(i+1));

	/* Delete first half elements. */
	for (i = 0; i < RND_NODES / 2; ++i)
		radix_del(&radix, i, true);

	/* Check both deleted and not-deleted elements. */
	for (i = 0; i < RND_NODES; ++i) {
		if (i < RND_NODES / 2)
			mu_check(radix_get(&radix, i) == NULL);
		else
			mu_check(radix_get(&radix, i) == (void *)(s64)(i+1));
	}

	/* Remove all elements. */
	for (i = RND_NODES / 2; i < RND_NODES; ++i)
		radix_del(&radix, i, true);

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
		// printf("i=%d key=%d value=%d\n",
		//       i, data[i].key, data[i].value);
	}

	/* TODO(MK): Add traversal test. */

	/* Delete some elements randomly. */
	for (i = 0; i < RND_NODES; ++i) {
		if (rand_value(100) & 0x1) {
			radix_del(&radix, data[i].key, true);
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
			radix_del(&radix, data[i].key, true);
			data[i].deleted = true;
		}
	}

	/* Delete all elements to be deleted. */
	for (i = 0; i < RND_NODES; ++i)
		mu_check(radix_get(&radix, data[i].key) == NULL);


	/* TEST 3 */
	/* Add 1:1 mapping to the radix. */
	for (i = 0; i < RND_NODES; ++i)
		radix_add(&radix, i, (void *)(s64)(i+1));

	/* Scan all. */
	struct radix_scan_cb_args args;
	args.next = 1;
	args.step = 1;
	err = radix_scan(&radix, 0, check_scan, &args);
	mu_check(err == 0);
	mu_check(args.next == RND_NODES + 1);

	/* Scan from the middle. */
	args.next = RND_NODES / 2;
	args.step = 1;
	err = radix_scan(&radix, args.next - 1, check_scan, &args);
	mu_check(err == 0);
	mu_check(args.next == RND_NODES + 1);

	/* Delete half keys. */
	for (i = 0; i < RND_NODES; i += 2)
		radix_del(&radix, i, true);

	args.next = 2;
	args.step = 2;
	err = radix_scan(&radix, 0, check_scan, &args);
	mu_check(err == 0);
	if (RND_NODES & 1)
		mu_check(args.next == RND_NODES + 1);
	else
		mu_check(args.next == RND_NODES + 2);

	/* Delete remaining keys. */
	for (i = 1; i < RND_NODES; i += 2)
		radix_del(&radix, i, true);

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
