/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>

#include "test_tools.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

int main()
{
        void *addr;

        info("Test unmap multiple vmrs begins...\n");

        addr = mmap(NULL,
                    PAGE_SIZE * 10,
                    PROT_READ,
                    MAP_ANONYMOUS | MAP_PRIVATE,
                    -1,
                    0);
        info("After mmap we can read this value: %c\n",
             ((char *)addr)[PAGE_SIZE * 8]);
        mprotect(addr, PAGE_SIZE * 2, PROT_READ | PROT_WRITE);
        mprotect(addr + PAGE_SIZE * 6, PAGE_SIZE * 4, PROT_READ | PROT_WRITE);
        ((char *)addr)[PAGE_SIZE * 8] = 'a';
        munmap(addr, PAGE_SIZE * 10);

        info("Should fail on following line...\n");
        ((char *)addr)[PAGE_SIZE * 8] = 'b';
        return 0;
}
