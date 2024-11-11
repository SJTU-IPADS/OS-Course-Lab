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

char *base_dir;

static inline void print_usage(void)
{
        printf("usage: fs_test_full.bin ./test_dir");
}

int main(int argc, char *argv[], char *envp[])
{
        int ret;
        struct stat stat_buf;
        char cwd[4096];

        if (argc >= 3) {
                print_usage();
                return 0;
        }

        if (argc == 2) {
                base_dir = argv[1];
        } else {
                base_dir = "/base_dir";
        }

        /* Check Dependencys */
#if (defined CHECK_TIME) && (defined HAVE_FTRUNCATE)      \
        && (defined HAVE_TRUNCATE) && (defined HAVE_PERM) \
        && (defined HAVE_SYMLINK) && (defined HAVE_CHFLAGS)
        printf("FileSystem full test is ready!\n");
#else
        printf("Dependencies are not set correctly!\n");
#endif

        /* Configure Print */
#ifdef FS_TEST_LINUX
        printf("Running FS Test on Linux\n");
#else
        printf("Running FS Test on ChCore\n");
#endif

#if CHECK_TIME
        printf("CHECK_TIME is opened\n");
#else
        printf("CHECK_TIME is closed\n");
#endif

        /* Prepare */
        fs_assert_noent(base_dir);
        ret = mkdir(base_dir, S_IRUSR | S_IWUSR | S_IXUSR);
        fs_assert_zero(ret);

        ret = lstat(base_dir, &stat_buf);
        fs_assert_zero(ret);
        fs_assert(stat_buf.st_mode & S_IFDIR);

        /* Store current work direcory */
        getcwd(cwd, 4096);
        fs_assert_zero(chdir(base_dir));

        /* Test: chcore-fstest */
        test_open();
        test_lseek();
        test_content();
        test_mmap();
        test_dup();
        test_mode();
        /* Test: pjdfstest-port */
        srand(time(NULL));
        run_pjdtest();

        /* Finish */
        printf("\n\nClean up!\n");
        fs_assert_zero(chdir(cwd));
        ret = rmdir(base_dir);
        fs_assert_zero(ret);
        fs_assert_noent(base_dir);
        printf("\nAll fs tests passed\n");

        return 0;
}
