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

#include "fs_test.h"
#include "fs_test_lib.h"
#include "fstest_utils.h"

/* Create a file and return fd */
int init_file(char *fname, int size)
{
        int fd, i;
        char buf[size];
        fd = open(fname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        fs_assert(fd > 0);
        memset(buf, 0, size);
        for (i = 0; i < size; i++) {
                buf[i] = i % 10;
        }
        fs_test_eq(write(fd, buf, size), size);
        return fd;
}

/* Close fd and unlink a file */
void deinit_file(int fd, char *fname)
{
        fs_test_eq(close(fd), 0);
        fs_test_eq(unlink(fname), 0);
        fs_assert_noent(fname);
}

/* For error printing in random_read/write_test */
static void index_tracing_print(int *index_tracing, int last_idx)
{
        int j;
        for (j = 0; j <= last_idx; ++j) {
                printf("%d\t", index_tracing[j]);
                if (j % 32 == 0)
                        printf("\n");
        }
}

/*
 * Check mmap region content randomly.
 * file's k-th byte should be (k % 10),
 * success on 0 , failue on -1
 */
int random_read_test(char *addr, int offset, int size)
{
        int index;
        int i;
        int *index_tracing; /* for error printing */

        index_tracing = (int *)malloc(READ_NUM * sizeof(int));

        srand(time(NULL));
        for (i = 0; i < READ_NUM; ++i) {
                index = rand() % size;
                index_tracing[i] = index;
                if (addr[index] != (index + offset) % 10) {
                        fs_test_log("failed when i=%d, index_tracing info:\n",
                                    i);
                        index_tracing_print(index_tracing, i);
                        free(index_tracing);
                        return -1;
                }
        }
        free(index_tracing);
        return 0;
}

/*
 * Write mmap region randomly and check using posix api.
 * success on 0 ,faile on -1
 */
int random_write_test(char *addr, int offset, int size, int fd)
{
        int index;
        int actual_index;
        int i;
        int *index_tracing;
        char buf[size];

        index_tracing = (int *)malloc(WRITE_NUM * sizeof(int));

        srand(time(NULL));
        for (i = 0; i < WRITE_NUM; i++) {
                index = rand() % size;
                index_tracing[i] = index;
                addr[index] = 'a';
                actual_index = index + offset;
                if (lseek(fd, actual_index, SEEK_SET) != actual_index) {
                        fs_test_log("failed when i=%d\n", i);
                        index_tracing_print(index_tracing, i);
                        free(index_tracing);
                        return -1;
                }
                if (addr[index] != 'a') {
                        fs_test_log("failed when i=%d\n", i);
                        index_tracing_print(index_tracing, i);
                        free(index_tracing);
                        return -1;
                }
                if ((read(fd, buf, 1) != 1)) {
                        fs_test_log("failed when i=%d\n", i);
                        index_tracing_print(index_tracing, i);
                        free(index_tracing);
                        return -1;
                }
                if (buf[0] != 'a') {
                        fs_test_log("failed when i=%d\n", i);
                        index_tracing_print(index_tracing, i);
                        free(index_tracing);
                        return -1;
                }
        }
        free(index_tracing);
        return 0;
}
