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

void mkdir_00()
{
        printf("\n");
        printf("Test from ../tests/mkdir/00.t:\n");
        fs_define_time(time);
        fs_define_time(atime);
        fs_define_time(mtime);
        fs_define_time(ctime);

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(mkdir(n1, 0755));
        char cdir[4096];
        getcwd(cdir, 4096);
        chdir(n1);

        fs_assert_zero(mkdir(n0, 0755));
        fs_assert_is_dir(n0);
        fs_check_mode(n0, 0755);
        fs_assert_zero(rmdir(n0));
        fs_assert_zero(mkdir(n0, 0151));
        fs_assert_is_dir(n0);
        fs_check_mode(n0, 0151);
        fs_assert_zero(rmdir(n0));
        fs_assert_zero(mkdir(n0, 0151));
        fs_assert_is_dir(n0);
        fs_check_mode(n0, 0100);
        fs_assert_zero(rmdir(n0));
        fs_assert_zero(mkdir(n0, 0345));
        fs_assert_is_dir(n0);
        fs_check_mode(n0, 0305);
        fs_assert_zero(rmdir(n0));
        fs_assert_zero(mkdir(n0, 0345));
        fs_assert_is_dir(n0);
        fs_check_mode(n0, 0244);
        fs_assert_zero(rmdir(n0));

        fs_assert_zero(chown(".", 65535, 65535));
        fs_assert_zero(mkdir(n0, 0755));
        FS_DO_NOTHING;
        fs_assert_zero(rmdir(n0));
        fs_assert_zero(mkdir(n0, 0755));
        FS_DO_NOTHING;
        fs_assert_zero(rmdir(n0));
        fs_assert_zero(mkdir(n0, 0755));
        FS_DO_NOTHING;
        fs_assert_zero(rmdir(n0));

        fs_assert_zero(chown(".", 0, 0));
        get_ctime(time, ".");
        sleep(1);
        fs_assert_zero(mkdir(n0, 0755));
        get_atime(atime, n0);
        fs_assert_time_lt(time, atime);
        get_mtime(mtime, n0);
        fs_assert_time_lt(time, mtime);
        get_ctime(ctime, n0);
        fs_assert_time_lt(time, ctime);
        get_mtime(mtime, ".");
        fs_assert_time_lt(time, mtime);
        get_ctime(ctime, ".");
        fs_assert_time_lt(time, ctime);
        fs_assert_zero(rmdir(n0));

        chdir(cdir);
        fs_assert_zero(rmdir(n1));

        chdir(cwd);
        printf("\n");
}

void mkdir_01()
{
        printf("\n");
        printf("Test from ../tests/mkdir/01.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        int fd4 = creat(path_join(n0, n1), 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_test_eq(-1, mkdir(path_join(path_join(n0, n1), "test"), 0755));
        fs_test_eq(ENOTDIR, errno);
        fs_assert_zero(unlink(path_join(n0, n1)));
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
        printf("\n");
}

void mkdir_02()
{
        printf("\n");
        printf("Test from ../tests/mkdir/02.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *nx = namegen_max();
        char *nxx = str_plus(nx, "x");

        fs_assert_zero(mkdir(nx, 0755));
        fs_assert_zero(rmdir(nx));
        fs_test_eq(-1, mkdir(nxx, 0755));
        fs_test_eq(ENAMETOOLONG, errno);

        chdir(cwd);
        printf("\n");
}

void mkdir_03()
{
        printf("\n");
        printf("Test from ../tests/mkdir/03.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *nx = dirgen_max();
        char *nxx = str_plus(nx, "x");

        mkdir_p_dirmax(nx);

        fs_assert_zero(mkdir(nx, 0755));
        fs_assert_zero(rmdir(nx));
        fs_test_eq(-1, mkdir(nxx, 0755));
        fs_test_eq(ENAMETOOLONG, errno);

        rmdir_r_dirmax(nx);

        chdir(cwd);
        printf("\n");
}

void mkdir_04()
{
        printf("\n");
        printf("Test from ../tests/mkdir/04.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        fs_test_eq(-1, mkdir(path_join(path_join(n0, n1), "test"), 0755));
        fs_test_eq(ENOENT, errno);
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
        printf("\n");
}

void mkdir_05()
{
        printf("\n");
        printf("Test from ../tests/mkdir/05.t:\n");
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
        fs_assert_zero(rmdir(path_join(n1, n2)));
        fs_test_eq(-1, mkdir(path_join(n1, n2), 0755));
        fs_test_eq(EACCES, errno);
        fs_assert_zero(mkdir(path_join(n1, n2), 0755));
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

void mkdir_06()
{
        printf("\n");
        printf("Test from ../tests/mkdir/06.t:\n");
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
        fs_assert_zero(rmdir(path_join(n1, n2)));
        fs_test_eq(-1, mkdir(path_join(n1, n2), 0755));
        fs_test_eq(EACCES, errno);
        fs_assert_zero(mkdir(path_join(n1, n2), 0755));
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

void mkdir_07()
{
        printf("\n");
        printf("Test from ../tests/mkdir/07.t:\n");
#if (HAVE_SYMLINK == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(symlink(n0, n1));
        fs_assert_zero(symlink(n1, n0));
        fs_test_eq(-1, mkdir(path_join(n0, "test"), 0755));
        fs_test_eq(ELOOP, errno);
        fs_test_eq(-1, mkdir(path_join(n1, "test"), 0755));
        fs_test_eq(ELOOP, errno);
        fs_assert_zero(unlink(n0));
        fs_assert_zero(unlink(n1));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void mkdir_08()
{
        printf("\n");
        printf("Test from ../tests/mkdir/08.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void mkdir_09()
{
        printf("\n");
        printf("Test from ../tests/mkdir/09.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void mkdir_10()
{
        printf("\n");
        printf("Test from ../tests/mkdir/10.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);

        int fd4 = creat(n0, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_test_eq(-1, mkdir(n0, 0755));
        fs_test_eq(EEXIST, errno);
        fs_assert_zero(unlink(n0));

        fs_assert_zero(mkdir(n0, 0));
        fs_test_eq(-1, mkdir(n0, 0755));
        fs_test_eq(EEXIST, errno);
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
        printf("\n");
}

void mkdir_11()
{
        printf("\n");
        printf("Test from ../tests/mkdir/11.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void mkdir_12()
{
        printf("\n");
        printf("Test from ../tests/mkdir/12.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        fs_test_eq(-1, mkdir((const char *)NULL, 0755));
        fs_test_eq(EFAULT, errno);
        fs_test_eq(-1, mkdir((const char *)NULL /* DEADCODE */, 0755));
        fs_test_eq(EFAULT, errno);

        chdir(cwd);
        printf("\n");
}
