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

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sched.h>
#include <sys/mman.h>

#define MALLOC_NUM      10

int main(int argc, char *argv[], char *envp[])
{
        char *str = "Hello, world!\n";
        char *bufs[MALLOC_NUM];
        int size;
        int i;
        int ret = 0;

        printf("%s\n", str);

        for (i = 0; i < argc; ++i) {
                printf("argv[%d]: %s\n", i, argv[i]);
        }

        size = strlen(str);
        bufs[0] = malloc(size + 1);
        memset(bufs[0], 0, size + 1);

        for (i = 0; i < size; ++i) {
                bufs[0][i] = str[i];
        }
        printf("bufs: %p, bufs content: %s\n", bufs[0], bufs[0]);
        free(bufs[0]);

        /* Malloc large memory */
        for (i = 0; i < MALLOC_NUM; ++i) {
                bufs[i] = malloc(0x1000);
                printf("large malloc addr: %p\n", bufs[i]);
        }
        for (i = 0; i < MALLOC_NUM; ++i)
                free(bufs[i]);

        /* Malloc small memory */
        for (i = 0; i < MALLOC_NUM; ++i) {
                bufs[i] = malloc(0x16);
                printf("small malloc addr: %p\n", bufs[i]);
        }
        for (i = 0; i < MALLOC_NUM; ++i)
                free(bufs[i]);

        /* mmap function */
        bufs[0] = mmap(0,
                   0x800,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS,
                   -1,
                   0);
        printf("mmap addr: %p\n", bufs[0]);

        for (i = 0; i < 0x800; ++i) {
                bufs[0][i] = i % 256;
        }

        bufs[1] = mmap(0,
                   0x1000,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS,
                   -1,
                   0);
        printf("mmap addr: %p\n", bufs[1]);

        bufs[1][0] = 'T';
        ret = mprotect(bufs[1], 0x1000, PROT_READ);
        assert(ret == 0);
        printf("read after mprotect: %c\n", bufs[1][0]);
#if 0
	/* Test mprotect: trigger error here */
	bufs[1][0] = 'X';
	printf("write after mprotect: %c\n", bufs[1][0]);
#endif

        ret = munmap(bufs[0], 0x1000);
        assert(ret == 0);
        ret = munmap(bufs[1], 0x1000);
        assert(ret == 0);
#if 0
	/* Test munmap: trigger error here */
	printf("read after mprotect: %c\n", bufs[1][0]);
#endif

        /* Affinity */
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(3, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        printf("set affinity return: %d\n", ret);

        CPU_ZERO(&mask);
        ret = sched_getaffinity(0, sizeof(mask), &mask);
        printf("get affinity return: %d\n", ret);

        for (i = 0; i < 128; i++) {
                if (CPU_ISSET(i, &mask)) {
                        printf("current affinity: %d\n", i);
                }
        }

        sched_yield();

        printf("Alive after yield!\n");
        printf("Hello world finished!\n");
        return 0;
}
