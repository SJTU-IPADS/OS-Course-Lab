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
#include <sys/mman.h>
#include <sys/time.h>
#include "perf_tools.h"

#define MMAP_COUNT PERF_NUM_SMALL
#define MMAP_VADDR 0x40000000 /* 1G */
#define MMAP_LEN   0x80000 /* 128K */

#define TEST_COUNT PERF_NUM_LARGE

unsigned long addrs[MMAP_COUNT];

void map_region(unsigned long start_address, unsigned long length, int count)
{
        void *addr;

        /* Actually, the mmap impl on ChCore will not refer to the input addr.
         */
        addr = (void *)start_address;
        addr = mmap(addr,
                    length,
                    PROT_READ | PROT_WRITE,
                    MAP_ANONYMOUS | MAP_PRIVATE,
                    -1,
                    0);
        // printf("%s: %p - %p\n", __func__, addr, addr + length);
        addrs[count] = (unsigned long)addr;
}

int main(int argc, char *argv[])
{
        perf_time_t *start_perf_time = malloc(sizeof(perf_time_t));
        perf_time_t *end_perf_time = malloc(sizeof(perf_time_t));
        int map_count;
        unsigned long map_addr;
        unsigned long map_len;

        map_addr = MMAP_VADDR;
        map_len = MMAP_LEN;

        for (map_count = 0; map_count < MMAP_COUNT; ++map_count) {
                map_region(map_addr, map_len, map_count);
                map_addr += map_len;
        }

        unsigned long start_addr, end_addr;
        unsigned long total_count = 0;

        record_time(start_perf_time);

        for (map_count = 0; map_count < MMAP_COUNT; ++map_count) {
                start_addr = addrs[map_count];
                end_addr = start_addr + MMAP_LEN;
                for (map_addr = start_addr; map_addr < end_addr;
                     map_addr += 0x1000) {
                        *(volatile unsigned long *)map_addr = map_addr;
                        total_count += 1;
                }
        }

        record_time(end_perf_time);
        info("Perf Pgfault Result:\n");
        print_perf_result(start_perf_time, end_perf_time, total_count);

        free(start_perf_time);
        free(end_perf_time);
        return 0;
}
