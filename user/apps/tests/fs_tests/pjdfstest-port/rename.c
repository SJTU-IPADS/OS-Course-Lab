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

void rename_00()
{
        printf("\n");
        printf("Test from ../tests/rename/00.t:\n");
        ino_t inode;

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);
        char *n2 = str_rand(6);
        char *n3 = str_rand(6);

        fs_assert_zero(mkdir(n3, 0755));
        char cdir[4096];
        getcwd(cdir, 4096);
        chdir(n3);

        int fd4 = creat(n0, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_assert_is_reg(n0);
        fs_check_mode(n0, 0644);
        fs_check_nlinks(n0, 1);
        get_inode(inode, n0);
        fs_assert_zero(rename(n0, n1));
        fs_assert_noent(n0);
        fs_assert_is_reg(n1);
        ino_t inode4;
        get_inode(inode4, n1);
        fs_assert_inum_fixed(inode4, inode);
        fs_check_mode(n1, 0644);
        fs_check_nlinks(n1, 1);
        fs_assert_zero(rename(n1, n2));
        fs_assert_noent(n1);
        fs_assert_is_reg(n2);
        ino_t inode8;
        get_inode(inode8, n2);
        fs_assert_inum_fixed(inode8, inode);
        fs_check_mode(n2, 0644);
        fs_check_nlinks(n2, 1);
        fs_assert_zero(unlink(n2));

        fs_assert_zero(mkdir(n0, 0755));
        fs_assert_is_dir(n0);
        fs_check_mode(n0, 0755);
        get_inode(inode, n0);
        fs_assert_zero(rename(n0, n1));
        fs_assert_noent(n0);
        fs_assert_is_dir(n1);
        fs_check_mode(n1, 0755);
        ino_t inode12;
        get_inode(inode12, n1);
        fs_assert_inum_fixed(inode12, inode);
        fs_assert_zero(rmdir(n1));

        chdir(cdir);
        fs_assert_zero(rmdir(n3));

        chdir(cwd);
        printf("\n");
}

void rename_01()
{
        printf("\n");
        printf("Test from ../tests/rename/01.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *nx = namegen_max();
        char *nxx = str_plus(nx, "x");

        int fd4 = creat(nx, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_assert_zero(rename(nx, n0));
        fs_assert_zero(rename(n0, nx));
        fs_assert_zero(unlink(nx));

        int fd8 = creat(n0, 0644);
        fs_assert(fd8 > 0);
        fs_assert_zero(close(fd8));
        fs_test_eq(-1, rename(n0, nxx));
        fs_test_eq(ENAMETOOLONG, errno);
        fs_assert_zero(unlink(n0));
        fs_test_eq(-1, rename(nxx, n0));
        fs_test_eq(ENAMETOOLONG, errno);

        chdir(cwd);
        printf("\n");
}

void rename_02()
{
        printf("\n");
        printf("Test from ../tests/rename/02.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *nx = dirgen_max();
        char *nxx = str_plus(nx, "x");

        mkdir_p_dirmax(nx);

        int fd4 = creat(n0, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_assert_zero(rename(n0, nx));
        fs_assert_zero(rename(nx, n0));
        fs_test_eq(-1, rename(n0, nxx));
        fs_test_eq(ENAMETOOLONG, errno);
        fs_assert_zero(unlink(n0));
        fs_test_eq(-1, rename(nxx, n0));
        fs_test_eq(ENAMETOOLONG, errno);

        rmdir_r_dirmax(nx);

        chdir(cwd);
        printf("\n");
}

void rename_03()
{
        printf("\n");
        printf("Test from ../tests/rename/03.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);
        char *n2 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        fs_test_eq(-1, rename(path_join(path_join(n0, n1), "test"), n2));
        fs_test_eq(ENOENT, errno);
        int fd4 = creat(n2, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_test_eq(-1, rename(n2, path_join(path_join(n0, n1), "test")));
        fs_test_eq(ENOENT, errno);
        fs_assert_zero(unlink(n2));
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
        printf("\n");
}

void rename_04()
{
        printf("\n");
        printf("Test from ../tests/rename/04.t:\n");
#if (HAVE_PERM == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);
        char *n2 = str_rand(6);
        char *n3 = str_rand(6);
        char *n4 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        char cdir[4096];
        getcwd(cdir, 4096);
        chdir(n0);

        fs_assert_zero(mkdir(n1, 0755));
        fs_assert_zero(chown(n1, 65534, 65534));
        fs_assert_zero(mkdir(n2, 0755));
        fs_assert_zero(chown(n2, 65534, 65534));
        int fd4 = creat(path_join(n1, n3), 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));

        fs_assert_zero(rename(path_join(n1, n3), path_join(n2, n4)));
        fs_assert_zero(rename(path_join(n2, n4), path_join(n1, n3)));

        fs_test_eq(-1, rename(path_join(n1, n3), path_join(n1, n4)));
        fs_test_eq(EACCES, errno);
        fs_test_eq(-1, rename(path_join(n1, n3), path_join(n2, n4)));
        fs_test_eq(EACCES, errno);

        fs_test_eq(-1, rename(path_join(n1, n3), path_join(n2, n4)));
        fs_test_eq(EACCES, errno);

        fs_assert_zero(unlink(path_join(n1, n3)));
        fs_assert_zero(rmdir(n1));
        fs_assert_zero(rmdir(n2));

        chdir(cdir);
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rename_05()
{
        printf("\n");
        printf("Test from ../tests/rename/05.t:\n");
#if (HAVE_PERM == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);
        char *n2 = str_rand(6);
        char *n3 = str_rand(6);
        char *n4 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        char cdir[4096];
        getcwd(cdir, 4096);
        chdir(n0);

        fs_assert_zero(mkdir(n1, 0755));
        fs_assert_zero(chown(n1, 65534, 65534));
        fs_assert_zero(mkdir(n2, 0755));
        fs_assert_zero(chown(n2, 65534, 65534));
        int fd4 = creat(path_join(n1, n3), 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));

        fs_assert_zero(rename(path_join(n1, n3), path_join(n2, n4)));
        fs_assert_zero(rename(path_join(n2, n4), path_join(n1, n3)));

        fs_test_eq(-1, rename(path_join(n1, n3), path_join(n2, n4)));
        fs_test_eq(EACCES, errno);
        fs_test_eq(-1, rename(path_join(n1, n3), path_join(n1, n4)));
        fs_test_eq(EACCES, errno);

        fs_assert_zero(unlink(path_join(n1, n3)));
        fs_assert_zero(rmdir(n1));
        fs_assert_zero(rmdir(n2));

        chdir(cdir);
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rename_06()
{
        printf("\n");
        printf("Test from ../tests/rename/06.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rename_07()
{
        printf("\n");
        printf("Test from ../tests/rename/07.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rename_08()
{
        printf("\n");
        printf("Test from ../tests/rename/08.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rename_09()
{
        printf("\n");
        printf("Test from ../tests/rename/09.t:\n");
#if (FS_TEST_IGNORE == 1) && (HAVE_PERM == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rename_10()
{
        printf("\n");
        printf("Test from ../tests/rename/10.t:\n");
#if (FS_TEST_IGNORE == 1) && (HAVE_PERM == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rename_11()
{
        printf("\n");
        printf("Test from ../tests/rename/11.t:\n");
#if (HAVE_SYMLINK == 1)

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);
        char *n2 = str_rand(6);

        fs_assert_zero(symlink(n0, n1));
        fs_assert_zero(symlink(n1, n0));
        fs_test_eq(-1, rename(path_join(n0, "test"), n2));
        fs_test_eq(ELOOP, errno);
        fs_test_eq(-1, rename(path_join(n1, "test"), n2));
        fs_test_eq(ELOOP, errno);
        int fd4 = creat(n2, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_test_eq(-1, rename(n2, path_join(n0, "test")));
        fs_test_eq(ELOOP, errno);
        fs_test_eq(-1, rename(n2, path_join(n1, "test")));
        fs_test_eq(ELOOP, errno);
        fs_assert_zero(unlink(n0));
        fs_assert_zero(unlink(n1));
        fs_assert_zero(unlink(n2));

        chdir(cwd);
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rename_12()
{
        printf("\n");
        printf("Test from ../tests/rename/12.t:\n");
#if (FS_TEST_IGNORE == 1) && (HAVE_LINK == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rename_13()
{
        printf("\n");
        printf("Test from ../tests/rename/13.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));

        int fd4 = creat(n1, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_test_eq(-1, rename(n0, n1));
        fs_test_eq(ENOTDIR, errno);
        fs_assert_is_dir(n0);
        fs_assert_is_reg(n1);
        fs_assert_zero(unlink(n1));

        fs_assert_zero(rmdir(n0));

        chdir(cwd);
        printf("\n");
}

void rename_14()
{
        printf("\n");
        printf("Test from ../tests/rename/14.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));

        int fd4 = creat(n1, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_test_eq(-1, rename(n1, n0));
        fs_test_eq(EISDIR, errno);
        fs_assert_is_dir(n0);
        fs_assert_is_reg(n1);
        fs_assert_zero(unlink(n1));

        fs_assert_zero(rmdir(n0));

        chdir(cwd);
        printf("\n");
}

void rename_15()
{
        printf("\n");
        printf("Test from ../tests/rename/15.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rename_16()
{
        printf("\n");
        printf("Test from ../tests/rename/16.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rename_17()
{
        printf("\n");
        printf("Test from ../tests/rename/17.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);

        int fd4 = creat(n0, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_test_eq(-1, rename(n0, (const char *)NULL));
        fs_test_eq(EFAULT, errno);
        fs_test_eq(-1, rename(n0, (const char *)NULL /* DEADCODE */));
        fs_test_eq(EFAULT, errno);
        fs_assert_zero(unlink(n0));
        fs_test_eq(-1, rename((const char *)NULL, n0));
        fs_test_eq(EFAULT, errno);
        fs_test_eq(-1, rename((const char *)NULL /* DEADCODE */, n0));
        fs_test_eq(EFAULT, errno);
        fs_test_eq(-1,
                   rename((const char *)NULL,
                          (const char *)NULL /* DEADCODE */));
        fs_test_eq(EFAULT, errno);
        fs_test_eq(-1,
                   rename((const char *)NULL /* DEADCODE */,
                          (const char *)NULL));
        fs_test_eq(EFAULT, errno);

        chdir(cwd);
        printf("\n");
}

void rename_18()
{
        printf("\n");
        printf("Test from ../tests/rename/18.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);
        char *n2 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        fs_assert_zero(mkdir(path_join(n0, n1), 0755));

        fs_test_eq(-1, rename(n0, path_join(n0, n1)));
        fs_test_eq(EINVAL, errno);
        fs_test_eq(-1, rename(n0, path_join(path_join(n0, n1), n2)));
        fs_test_eq(EINVAL, errno);

        fs_assert_zero(rmdir(path_join(n0, n1)));
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
        printf("\n");
}

void rename_19()
{
        printf("\n");
        printf("Test from ../tests/rename/19.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);
        char *n2 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        fs_assert_zero(mkdir(path_join(n0, n1), 0755));

        fs_test_eq(-1, rename(path_join(path_join(n0, n1), "."), n2));
        fs_assert(errno == EINVAL || errno == EBUSY);
        fs_test_eq(-1, rename(path_join(path_join(n0, n1), ".."), n2));
        fs_assert(errno == EINVAL || errno == EBUSY);

        fs_assert_zero(rmdir(path_join(n0, n1)));
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
        printf("\n");
}

void rename_20()
{
        printf("\n");
        printf("Test from ../tests/rename/20.t:\n");

        char cwd[4096];
        getcwd(cwd, 4096);

        char *n0 = str_rand(6);
        char *n1 = str_rand(6);
        char *n2 = str_rand(6);

        fs_assert_zero(mkdir(n0, 0755));
        fs_assert_zero(mkdir(n1, 0755));

        int fd4 = creat(path_join(n1, n2), 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        fs_test_eq(-1, rename(n0, n1));
        fs_assert(errno == EEXIST || errno == ENOTEMPTY);
        fs_assert_zero(unlink(path_join(n1, n2)));

        fs_assert_zero(mkdir(path_join(n1, n2), 0));
        fs_test_eq(-1, rename(n0, n1));
        fs_assert(errno == EEXIST || errno == ENOTEMPTY);
        fs_assert_zero(rmdir(path_join(n1, n2)));

        fs_assert_zero(rmdir(n1));
        fs_assert_zero(rmdir(n0));

        chdir(cwd);
        printf("\n");
}

void rename_21()
{
        printf("\n");
        printf("Test from ../tests/rename/21.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}

void rename_22()
{
        printf("\n");
        printf("Test from ../tests/rename/22.t:\n");
        fs_define_time(ctime1);
        fs_define_time(ctime2);

        char cwd[4096];
        getcwd(cwd, 4096);

        char *src = str_rand(6);
        char *dst = str_rand(6);
        char *parent = str_rand(6);

        fs_assert_zero(mkdir(parent, 0755));
        char cdir[4096];
        getcwd(cdir, 4096);
        chdir(parent);

        int fd4 = creat(src, 0644);
        fs_assert(fd4 > 0);
        fs_assert_zero(close(fd4));
        get_ctime(ctime1, src);
        sleep(1);
        fs_assert_zero(rename(src, dst));
        get_ctime(ctime2, dst);
        fs_assert_time_lt(ctime1, ctime2);
        fs_assert_zero(unlink(dst));

        fs_assert_zero(mkdir(src, 0));
        get_ctime(ctime1, src);
        sleep(1);
        fs_assert_zero(rename(src, dst));
        get_ctime(ctime2, dst);
        fs_assert_time_lt(ctime1, ctime2);
        fs_assert_zero(rmdir(dst));

        chdir(cdir);
        fs_assert_zero(rmdir(parent));

        chdir(cwd);
        printf("\n");
}

void rename_23()
{
        printf("\n");
        printf("Test from ../tests/rename/23.t:\n");
#if (FS_TEST_IGNORE == 1)
        FS_DO_NOTHING;
#else
        printf(COLOR_YELLOW "This case is skipped" COLOR_DEFAULT);
#endif
        printf("\n");
}
