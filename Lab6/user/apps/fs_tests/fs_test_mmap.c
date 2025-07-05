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

#include "fs_tools/fs_test.h"
#include "fs_tools/fs_test_lib.h"
#include "fs_tools/fstest_utils.h"
#include "unistd.h"
#include <assert.h>

static void generate_random_file_properties(int *filesize, int *offset)
{
        int a, b;
        srand((unsigned int)clock());
        /* guarantee that file larger than one page */
        a = TEST_FILE_SIZE;
        b = rand() % (a / PAGE_SIZE);
        b *= PAGE_SIZE;
        *filesize = a;
        *offset = b;
}

int main(int argc, char *argv[])
{
        int fd, offset, filesize, mmap_size;
        char *addr;
        char *fname;

        assert(argc == 2);
        chdir(argv[1]); /* ensuring isolation in test_fs_concurrency */

        srand(time(NULL));
        generate_random_file_properties(&filesize, &offset);
        fname = "test_mmap.txt";
        fd = init_file(fname, filesize);
        mmap_size = PAGE_SIZE * ((filesize - offset) / PAGE_SIZE);
        addr = mmap(NULL,
                    mmap_size,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE,
                    fd,
                    offset);
        fs_test_eq(random_read_test(addr, offset, mmap_size), 0);

        /* Unlink will be trigger in main process */
        close(fd);
        return 0;
}
