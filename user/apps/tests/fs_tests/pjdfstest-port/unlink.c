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

void unlink_00()
{
        printf("\n");
        printf("Test from ../tests/unlink/00.t:\n");
        fs_define_time(time);
        fs_define_time(mtime);
        fs_define_time(ctime);

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);
        char *n2 = str_rand(6);

        fs_assert_zero(mkdir(n2, 0755));
        char cdir[4096];
        getcwd(cdir, 4096);
        chdir(n2);

        int fd4 = creat(n0, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_assert_is_reg(n0);
        fs_assert_zero(unlink(n0));
        fs_assert_noent(n0);

        fs_assert_zero(mkdir(n0, 0755));
        int fd8 = creat(path_join(n0, n1), 0644);
        fs_assert(fd8 > 0);
        fs_assert_zero(close(fd8));
        get_ctime(time, n0);
        sleep(1);
        fs_assert_zero(unlink(path_join(n0, n1)));
        get_mtime(mtime, n0);
        fs_assert_time_lt(time, mtime);
        get_ctime(ctime, n0);
        fs_assert_time_lt(time, ctime);
        fs_assert_zero(rmdir(n0));

        chdir(cdir);
        fs_assert_zero(rmdir(n2));

        chdir(cwd);
        printf("\n");
}

void unlink_01()
{
        printf("\n");
        printf("Test from ../tests/unlink/01.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        int fd4 = creat(path_join(n0, n1), 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_test_eq(-1, unlink(path_join(path_join(n0, n1), "test")));
        fs_test_eq(ENOTDIR, errno);
        fs_assert_zero(unlink(path_join(n0, n1)));
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
        printf("\n");
}

void unlink_02()
{
        printf("\n");
        printf("Test from ../tests/unlink/02.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *nx = namegen_max();
        char *nxx = str_plus(nx, "x");

        int fd4 = creat(nx, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_assert_zero(unlink(nx));
        fs_test_eq(-1, unlink(nx));
        fs_test_eq(ENOENT, errno);
        fs_test_eq(-1, unlink(nxx));
        fs_test_eq(ENAMETOOLONG, errno);

        chdir(cwd);
        printf("\n");
}

void unlink_03()
{
        printf("\n");
        printf("Test from ../tests/unlink/03.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *nx = dirgen_max();
        char *nxx = str_plus(nx, "x");

        mkdir_p_dirmax(nx);

        int fd4 = creat(nx, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_assert_zero(unlink(nx));
        fs_test_eq(-1, unlink(nx));
        fs_test_eq(ENOENT, errno);
        fs_test_eq(-1, unlink(nxx));
        fs_test_eq(ENAMETOOLONG, errno);

        rmdir_r_dirmax(nx);

        chdir(cwd);
        printf("\n");
}

void unlink_04()
{
        printf("\n");
        printf("Test from ../tests/unlink/04.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        int fd4 = creat(n0, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_assert_zero(unlink(n0));
        fs_test_eq(-1, unlink(n0));
        fs_test_eq(ENOENT, errno);
        fs_test_eq(-1, unlink(n1));
        fs_test_eq(ENOENT, errno);

        chdir(cwd);
        printf("\n");
}

void unlink_05()
{
        printf("\n");
        printf("Test from ../tests/unlink/05.t:\n");
#if (HAVE_PERM == 1)

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
        fs_test_eq(-1, unlink(path_join(n1, n2)));
        fs_test_eq(EACCES, errno);
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

void unlink_06()
{
        printf("\n");
        printf("Test from ../tests/unlink/06.t:\n");
#if (HAVE_PERM == 1)

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
        fs_test_eq(-1, unlink(path_join(n1, n2)));
        fs_test_eq(EACCES, errno);
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

void unlink_07()
{
        printf("\n");
        printf("Test from ../tests/unlink/07.t:\n");
#if (HAVE_SYMLINK == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(symlink(n0, n1));
        fs_assert_zero(symlink(n1, n0));
        fs_test_eq(-1, unlink(path_join(n0, "test")));
        fs_test_eq(ELOOP, errno);
        fs_test_eq(-1, unlink(path_join(n1, "test")));
        fs_test_eq(ELOOP, errno);
        fs_assert_zero(unlink(n0));
        fs_assert_zero(unlink(n1));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void unlink_08()
{
        printf("\n");
        printf("Test from ../tests/unlink/08.t:\n");
#if (HAVE_PERM == 1) && (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void unlink_09()
{
        printf("\n");
        printf("Test from ../tests/unlink/09.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void unlink_10()
{
        printf("\n");
        printf("Test from ../tests/unlink/10.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void unlink_11()
{
        printf("\n");
        printf("Test from ../tests/unlink/11.t:\n");
#if (HAVE_PERM == 1) && (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void unlink_12()
{
        printf("\n");
        printf("Test from ../tests/unlink/12.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void unlink_13()
{
        printf("\n");
        printf("Test from ../tests/unlink/13.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        fs_test_eq(-1, unlink((const char *)NULL));
        fs_test_eq(EFAULT, errno);
        fs_test_eq(-1, unlink((const char *)NULL /* DEADCODE */));
        fs_test_eq(EFAULT, errno);

        chdir(cwd);
        printf("\n");
}

void unlink_14()
{
        printf("\n");
        printf("Test from ../tests/unlink/14.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n2 = str_rand(6);

        fs_assert_zero(mkdir(n2, 0755));
        char cdir[4096];
        getcwd(cdir, 4096);
        chdir(n2);

        int fd4 = creat(n0, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        int fd8 = open(n0, O_WRONLY);
        fs_assert(fd8 > 0);
        fs_test_eq(13, write(fd8, "Hello, World!", 13));
        fs_assert_zero(close(fd8));
        int fd12 = open(n0, O_RDONLY);
        fs_assert_zero(unlink(n0));
        struct stat tmp_st;
        fs_assert_zero(fstat(fd12, &tmp_st));
        fs_assert_zero(tmp_st.st_nlink);
        fs_assert_zero(close(fd12));

        int fd16 = creat(n0, 0644);
        fs_assert(fd16 > 0);
        fs_assert_zero(close(fd16));
        int fd20 = open(n0, O_RDWR);
        fs_assert(fd20 > 0);
        fs_test_eq(13, write(fd20, "Hello, World!", 13));
        fs_assert_zero(unlink(n0));
        fs_assert_zero(lseek(fd20, 0, SEEK_SET));
        char buf12[30];
        fs_test_eq(13, read(fd20, buf12, 13));
        fs_assert_zero(strncmp("Hello, World!", buf12, 13));

        chdir(cdir);
        fs_assert_zero(rmdir(n2));

        chdir(cwd);
        printf("\n");
}
