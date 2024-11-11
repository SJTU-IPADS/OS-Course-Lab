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

#include <stdlib.h>
#include <minunit.h>
#include <pthread.h>
#include <time.h>

#include "test_redef.h"

#include <chcore/ring_buffer.h>

struct test_msg {
        badge_t badge;
        int exitcode;
        int padding;
};

int set_n_msg(struct ring_buffer *ring_buf, void *msg, int num)
{
        int i = 0;
        while (i < num) {
                if (!if_buffer_full(ring_buf)) {
                        set_one_msg(ring_buf, msg);
                        i++;
                } else {
                        return i;
                }
        }
        return i;
}

int get_n_msg(struct ring_buffer *ring_buf, struct test_msg *msg, int num)
{
        int i = 0;
        struct test_msg tmp;
        while (i < num) {
                if (get_one_msg(ring_buf, &tmp)) {
                        *msg = tmp;
                        i++;
                } else
                        return i;
        }
        return i;
}

int get_n_int(struct ring_buffer *ring_buf, int *msg, int num)
{
        int i = 0;
        int tmp = -1;
        while (i < num) {
                if (get_one_msg(ring_buf, &tmp)) {
                        *msg = tmp;
                        i++;
                } else
                        return i;
        }
        return i;
}

static void *test_producer(void *arg)
{
        srand((unsigned)(time(NULL)));
        struct ring_buffer *ring_buf = (struct ring_buffer *)arg;
        int count = 0;
        int i, j;
        int x = 1;
        while (count < 100) {
                i = rand() % 10;
                if (count + i <= 100) {
                        j = set_n_msg(ring_buf, &x, i);
                        count += j;
                } else {
                        i = 100 - count;
                        j = set_n_msg(ring_buf, &x, i);
                        count += j;
                }
        }
        return NULL;
}

static void *test_consumer(void *arg)
{
        srand((unsigned)(time(NULL)));
        struct ring_buffer *ring_buf = (struct ring_buffer *)arg;
        int count = 0;
        int i, j;
        int x = 1;
        while (count < 100) {
                i = rand() % 10;
                if (count + i <= 100) {
                        j = get_n_int(ring_buf, &x, i);
                        count += j;
                } else {
                        i = 100 - count;
                        j = get_n_int(ring_buf, &x, i);
                        count += j;
                }
        }
        return NULL;
}

MU_TEST(test_producer_and_consumer)
{
        struct ring_buffer *ring_buf;
        ring_buf = new_ringbuffer(50, sizeof(int));
        mu_check(ring_buf);
        int x = 0;
        int y = 0;
        pthread_t thread_id[2];
        pthread_create(&thread_id[0], NULL, test_producer, ring_buf);
        pthread_create(&thread_id[1], NULL, test_consumer, ring_buf);
        pthread_join(thread_id[0], NULL);
        pthread_join(thread_id[1], NULL);
        free_ringbuffer(ring_buf);
}

MU_TEST(test_int)
{
        struct ring_buffer *ring_buf;
        ring_buf = new_ringbuffer(50, sizeof(int));
        mu_check(ring_buf);
        int x = 0;
        int y = -1;
        int i = 0;
        while (i < 100) {
                x = i;
                mu_check(set_n_msg(ring_buf, &x, 13) == 13);
                mu_check(get_n_int(ring_buf, &y, 13) == 13);
                mu_check(y == i);
                i++;
        }
        free_ringbuffer(ring_buf);
}

MU_TEST(test_msg)
{
        srand((unsigned)(time(NULL)));
        struct ring_buffer *test_msg_buffer;
        test_msg_buffer = new_ringbuffer(20, sizeof(struct test_msg));
        struct test_msg tmp;
        struct test_msg msg;
        int i = 0;
        while (i < 100) {
                tmp.badge = rand() % INT16_MAX;
                tmp.exitcode = rand() % INT16_MAX;
                set_one_msg(test_msg_buffer, &tmp);
                get_one_msg(test_msg_buffer, &msg);
                mu_check(tmp.badge == msg.badge);
                mu_check(tmp.exitcode == msg.exitcode);
                i++;
        }
        free_ringbuffer(test_msg_buffer);
}

MU_TEST(test_bufferfull)
{
        struct ring_buffer *full_ring_buf;
        full_ring_buf = new_ringbuffer(16, sizeof(struct test_msg));
        struct test_msg q;
        struct test_msg msg;
        q.badge = 1;
        q.exitcode = 0;
        mu_check(set_n_msg(full_ring_buf, &q, 13) == 13);
        mu_check(set_n_msg(full_ring_buf, &q, 13) == 2);
        mu_check(get_n_msg(full_ring_buf, &msg, 10) == 10);
        mu_check(set_n_msg(full_ring_buf, &q, 13) == 10);
        mu_check(get_n_msg(full_ring_buf, &msg, 16) == 15);
        mu_check(set_n_msg(full_ring_buf, &q, 13) == 13);
        mu_check(set_n_msg(full_ring_buf, &q, 13) == 2);
        free_ringbuffer(full_ring_buf);
}

MU_TEST_SUITE(test_suite)
{
        MU_RUN_TEST(test_int);
        MU_RUN_TEST(test_msg);
        MU_RUN_TEST(test_bufferfull);
        MU_RUN_TEST(test_producer_and_consumer);
}

int main(int argc, char *argv[])
{
        MU_RUN_SUITE(test_suite);
        MU_REPORT();
        return minunit_status;
}
