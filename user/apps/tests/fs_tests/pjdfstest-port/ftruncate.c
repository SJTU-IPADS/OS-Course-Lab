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


/*
 * This file is generated automately, DO NOT modify it manually
 */
#include "fs_test.h"
#include "fs_test_lib.h"

void ftruncate_00()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/00.t:\n");
#if (HAVE_FTRUNCATE == 1)
        fs_define_time(ctime1);
        fs_define_time(ctime2);

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(mkdir(n1, 0755));
        char cdir[4096];
        getcwd(cdir, 4096);
        chdir(n1);

        int fd4 = creat(n0, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        int fd8 = open(n0, O_RDWR);
        fs_assert(fd8 > 0);
        fs_assert_zero(ftruncate(fd8, 1234567));
        fs_assert_zero(close(fd8));
        fs_assert_reg_size(n0, 1234567);
        int fd12 = open(n0, O_WRONLY);
        fs_assert(fd12 > 0);
        fs_assert_zero(ftruncate(fd12, 567));
        fs_assert_zero(close(fd12));
        fs_assert_reg_size(n0, 567);
        fs_assert_zero(unlink(n0));

        char *buf4 = (char *)malloc(12345);
        int fd16 = open(n0, O_CREAT | O_RDWR, 0644);
        fs_assert(fd16 > 0);
        fs_test_eq(write(fd16, buf4, 12345), 12345);
        fs_assert_zero(close(fd16));
        free(buf4);
        int fd20 = open(n0, O_RDWR);
        fs_assert(fd20 > 0);
        fs_assert_zero(ftruncate(fd20, 23456));
        fs_assert_zero(close(fd20));
        fs_assert_reg_size(n0, 23456);
        int fd24 = open(n0, O_WRONLY);
        fs_assert(fd24 > 0);
        fs_assert_zero(ftruncate(fd24, 1));
        fs_assert_zero(close(fd24));
        fs_assert_reg_size(n0, 1);
        fs_assert_zero(unlink(n0));

        int fd28 = creat(n0, 0644);
        fs_assert(fd28 > 0);
        fs_assert_zero(close(fd28));
        get_ctime(ctime1, n0);
        sleep(1);
        int fd32 = open(n0, O_RDWR);
        fs_assert(fd32 > 0);
        fs_assert_zero(ftruncate(fd32, 123));
        fs_assert_zero(close(fd32));
        get_ctime(ctime2, n0);
        fs_assert_time_lt(ctime1, ctime2);
        fs_assert_zero(unlink(n0));

        int fd36 = creat(n0, 0644);
        fs_assert(fd36 > 0);
        fs_assert_zero(close(fd36));
        get_ctime(ctime1, n0);
        sleep(1);
        int fd40 = open(n0, O_RDONLY);
        fs_assert(fd40 > 0);
        fs_test_eq(-1, ftruncate(fd40, 123));
        fs_test_eq(EINVAL, errno);
        fs_assert_zero(close(fd40));
        get_ctime(ctime2, n0);
        fs_assert_time_eq(ctime1, ctime2);
        fs_assert_zero(unlink(n0));

        int fd44 = open(n0, O_CREAT | O_RDWR, 0644);
        fs_assert(fd44 > 0);
        fs_assert_zero(ftruncate(fd44, 0));
        fs_assert_zero(close(fd44));
        fs_assert_zero(unlink(n0));
        int fd48 = open(n0, O_CREAT | O_RDWR, 0644);
        fs_assert(fd48 > 0);
        fs_assert_zero(ftruncate(fd48, 0));
        fs_assert_zero(close(fd48));
        fs_assert_zero(unlink(n0));

        chdir(cdir);
        fs_assert_zero(rmdir(n1));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void ftruncate_01()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/01.t:\n");
#if (HAVE_TRUNCATE == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        int fd4 = creat(path_join(n0, n1), 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_test_eq(-1, truncate(path_join(path_join(n0, n1), "test"), 123));
        fs_test_eq(ENOTDIR, errno);
        fs_assert_zero(unlink(path_join(n0, n1)));
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void ftruncate_02()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/02.t:\n");
#if (HAVE_TRUNCATE == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *nx = namegen_max();
        char *nxx = str_plus(nx, "x");

        int fd4 = creat(nx, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_assert_zero(truncate(nx, 123));
        fs_assert_reg_size(nx, 123);
        fs_assert_zero(unlink(nx));
        fs_test_eq(-1, truncate(nxx, 123));
        fs_test_eq(ENAMETOOLONG, errno);

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void ftruncate_03()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/03.t:\n");
#if (HAVE_TRUNCATE == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *nx = dirgen_max();
        char *nxx = str_plus(nx, "x");

        mkdir_p_dirmax(nx);

        int fd4 = creat(nx, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_assert_zero(truncate(nx, 123));
        fs_assert_reg_size(nx, 123);
        fs_assert_zero(unlink(nx));
        fs_test_eq(-1, truncate(nxx, 123));
        fs_test_eq(ENAMETOOLONG, errno);

        rmdir_r_dirmax(nx);

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void ftruncate_04()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/04.t:\n");
#if (HAVE_TRUNCATE == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        fs_test_eq(-1, truncate(path_join(path_join(n0, n1), "test"), 123));
        fs_test_eq(ENOENT, errno);
        fs_test_eq(-1, truncate(path_join(n0, n1), 123));
        fs_test_eq(ENOENT, errno);
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void ftruncate_05()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/05.t:\n");
#if (HAVE_TRUNCATE == 1) && (HAVE_PERM == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);
        char *n2 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        char cdir[4096];
        getcwd(cdir, 4096);
        chdir(n0);
        fs_assert_zero(mkdir(n1, 0755));
        fs_assert_zero(chown(n1, 65534, 65534));
        int fd4 = creat(path_join(n1, n2), 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_assert_zero(truncate(path_join(n1, n2), 123));
        fs_assert_reg_size(path_join(n1, n2), 123);
        fs_test_eq(-1, truncate(path_join(n1, n2), 1234));
        fs_test_eq(EACCES, errno);
        fs_assert_reg_size(path_join(n1, n2), 123);
        fs_assert_zero(truncate(path_join(n1, n2), 1234));
        fs_assert_reg_size(path_join(n1, n2), 1234);
        fs_assert_zero(unlink(path_join(n1, n2)));
        fs_assert_zero(rmdir(n1));
        chdir(cdir);
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void ftruncate_06()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/06.t:\n");
#if (HAVE_TRUNCATE == 1) && (HAVE_PERM == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        char cdir[4096];
        getcwd(cdir, 4096);
        chdir(n0);
        int fd4 = creat(n1, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_test_eq(-1, truncate(n1, 123));
        fs_test_eq(EACCES, errno);
        fs_assert_zero(chown(n1, 65534, 65534));
        fs_test_eq(-1, truncate(n1, 123));
        fs_test_eq(EACCES, errno);
        fs_assert_zero(unlink(n1));
        chdir(cdir);
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void ftruncate_07()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/07.t:\n");
#if (HAVE_TRUNCATE == 1) && (HAVE_SYMLINK == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(symlink(n0, n1));
        fs_assert_zero(symlink(n1, n0));
        fs_test_eq(-1, truncate(path_join(n0, "test"), 123));
        fs_test_eq(ELOOP, errno);
        fs_test_eq(-1, truncate(path_join(n1, "test"), 123));
        fs_test_eq(ELOOP, errno);
        fs_assert_zero(unlink(n0));
        fs_assert_zero(unlink(n1));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void ftruncate_08()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/08.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void ftruncate_09()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/09.t:\n");
#if (HAVE_TRUNCATE == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        fs_test_eq(-1, truncate(n0, 123));
        fs_test_eq(EISDIR, errno);
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void ftruncate_10()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/10.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void ftruncate_11()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/11.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void ftruncate_12()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/12.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void ftruncate_13()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/13.t:\n");
#if (HAVE_FTRUNCATE == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);

        int fd4 = creat(n0, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        int fd8 = open(n0, O_RDWR);
        fs_assert(fd8 > 0);
        fs_test_eq(-1, ftruncate(fd8, -1));
        fs_test_eq(EINVAL, errno);
        fs_assert_zero(close(fd8));
        int fd12 = open(n0, O_WRONLY);
        fs_assert(fd12 > 0);
        fs_test_eq(-1, ftruncate(fd12, -999999));
        fs_test_eq(EINVAL, errno);
        fs_assert_zero(close(fd12));
        fs_assert_zero(unlink(n0));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void ftruncate_14()
{
        printf("\n");
        printf("Test from ../tests/ftruncate/14.t:\n");
#if (HAVE_TRUNCATE == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        fs_test_eq(-1, truncate((const char *)NULL, 123));
        fs_test_eq(EFAULT, errno);
        fs_test_eq(-1, truncate((const char *)NULL /* DEADCODE */, 123));
        fs_test_eq(EFAULT, errno);

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}
