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

void rmdir_00()
{
        printf("\n");
        printf("Test from ../tests/rmdir/00.t:\n");
        fs_define_time(time);
        fs_define_time(mtime);
        fs_define_time(ctime);

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        fs_assert_is_dir(n0);
        fs_assert_zero(rmdir(n0));
        fs_assert_noent(n0);

        fs_assert_zero(mkdir(n0, 0755));
        fs_assert_zero(mkdir(path_join(n0, n1), 0755));
        get_ctime(time, n0);
        sleep(1);
        fs_assert_zero(rmdir(path_join(n0, n1)));
        get_mtime(mtime, n0);
        fs_assert_time_lt(time, mtime);
        get_ctime(ctime, n0);
        fs_assert_time_lt(time, ctime);
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
        printf("\n");
}

void rmdir_01()
{
        printf("\n");
        printf("Test from ../tests/rmdir/01.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        int fd4 = creat(path_join(n0, n1), 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_test_eq(-1, rmdir(path_join(path_join(n0, n1), "test")));
        fs_test_eq(ENOTDIR, errno);
        fs_assert_zero(unlink(path_join(n0, n1)));
        fs_assert_zero(rmdir(n0));

        int fd8 = creat(n0, 0644);
        fs_assert(fd8 > 0);
        fs_assert_zero(close(fd8));
        fs_test_eq(-1, rmdir(n0));
        fs_test_eq(ENOTDIR, errno);
        fs_assert_zero(unlink(n0));

        chdir(cwd);
        printf("\n");
}

void rmdir_02()
{
        printf("\n");
        printf("Test from ../tests/rmdir/02.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *nx = namegen_max();
        char *nxx = str_plus(nx, "x");

        fs_assert_zero(mkdir(nx, 0755));
        fs_assert_zero(rmdir(nx));
        fs_test_eq(-1, rmdir(nx));
        fs_test_eq(ENOENT, errno);
        fs_test_eq(-1, rmdir(nxx));
        fs_test_eq(ENAMETOOLONG, errno);

        chdir(cwd);
        printf("\n");
}

void rmdir_03()
{
        printf("\n");
        printf("Test from ../tests/rmdir/03.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *nx = dirgen_max();
        char *nxx = str_plus(nx, "x");

        mkdir_p_dirmax(nx);

        fs_assert_zero(mkdir(nx, 0755));
        fs_assert_is_dir(nx);
        fs_check_mode(nx, 0755);
        fs_assert_zero(rmdir(nx));
        fs_test_eq(-1, rmdir(nx));
        fs_test_eq(ENOENT, errno);
        fs_test_eq(-1, rmdir(nxx));
        fs_test_eq(ENAMETOOLONG, errno);

        rmdir_r_dirmax(nx);

        chdir(cwd);
        printf("\n");
}

void rmdir_04()
{
        printf("\n");
        printf("Test from ../tests/rmdir/04.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        fs_assert_zero(rmdir(n0));
        fs_test_eq(-1, rmdir(n0));
        fs_test_eq(ENOENT, errno);
        fs_test_eq(-1, rmdir(n1));
        fs_test_eq(ENOENT, errno);

        chdir(cwd);
        printf("\n");
}

void rmdir_05()
{
        printf("\n");
        printf("Test from ../tests/rmdir/05.t:\n");
#if (HAVE_SYMLINK == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(symlink(n0, n1));
        fs_assert_zero(symlink(n1, n0));
        fs_test_eq(-1, rmdir(path_join(n0, "test")));
        fs_test_eq(ELOOP, errno);
        fs_test_eq(-1, rmdir(path_join(n1, "test")));
        fs_test_eq(ELOOP, errno);
        fs_assert_zero(unlink(n0));
        fs_assert_zero(unlink(n1));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rmdir_06()
{
        printf("\n");
        printf("Test from ../tests/rmdir/06.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        int fd4 = creat(path_join(n0, n1), 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_test_eq(-1, rmdir(n0));
        fs_assert(errno == EEXIST || errno == ENOTEMPTY);
        fs_assert_zero(unlink(path_join(n0, n1)));

        fs_assert_zero(mkdir(path_join(n0, n1), 0));
        fs_test_eq(-1, rmdir(n0));
        fs_assert(errno == EEXIST || errno == ENOTEMPTY);
        fs_assert_zero(rmdir(path_join(n0, n1)));

        fs_assert_zero(rmdir(n0));

        chdir(cwd);
        printf("\n");
}

void rmdir_07()
{
        printf("\n");
        printf("Test from ../tests/rmdir/07.t:\n");
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
        fs_assert_zero(mkdir(path_join(n1, n2), 0755));
        fs_test_eq(-1, rmdir(path_join(n1, n2)));
        fs_test_eq(EACCES, errno);
        fs_assert_zero(rmdir(path_join(n1, n2)));
        fs_assert_zero(rmdir(n1));
        chdir(cdir);
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rmdir_08()
{
        printf("\n");
        printf("Test from ../tests/rmdir/08.t:\n");
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
        fs_assert_zero(mkdir(path_join(n1, n2), 0755));
        fs_test_eq(-1, rmdir(path_join(n1, n2)));
        fs_test_eq(EACCES, errno);
        fs_assert_zero(rmdir(path_join(n1, n2)));
        fs_assert_zero(rmdir(n1));
        chdir(cdir);
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rmdir_09()
{
        printf("\n");
        printf("Test from ../tests/rmdir/09.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rmdir_10()
{
        printf("\n");
        printf("Test from ../tests/rmdir/10.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rmdir_11()
{
        printf("\n");
        printf("Test from ../tests/rmdir/11.t:\n");
#if (HAVE_PERM == 1) && (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rmdir_12()
{
        printf("\n");
        printf("Test from ../tests/rmdir/12.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rmdir_13()
{
        printf("\n");
        printf("Test from ../tests/rmdir/13.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rmdir_14()
{
        printf("\n");
        printf("Test from ../tests/rmdir/14.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rmdir_15()
{
        printf("\n");
        printf("Test from ../tests/rmdir/15.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        fs_test_eq(-1, rmdir((const char *)NULL));
        fs_test_eq(EFAULT, errno);
        fs_test_eq(-1, rmdir((const char *)NULL /* DEADCODE */));
        fs_test_eq(EFAULT, errno);

        chdir(cwd);
        printf("\n");
}
