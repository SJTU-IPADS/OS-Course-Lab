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
        info("Test mprotect begins...\n");

        void *addr = mmap(NULL,
                          PAGE_SIZE * 10,
                          PROT_READ | PROT_WRITE,
                          MAP_ANONYMOUS | MAP_PRIVATE,
                          -1,
                          0);
        ((char *)addr)[0] = 'a';
        mprotect(addr, PAGE_SIZE * 6, PROT_READ);
        ((char *)addr)[PAGE_SIZE * 7] = 'b';

        info("Should fail on following line...\n");
        ((char *)addr)[0] = 'c';
        return 0;
}
