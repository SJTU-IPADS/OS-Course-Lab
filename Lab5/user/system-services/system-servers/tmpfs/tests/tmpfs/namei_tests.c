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

#include "chcore/container/hashtable.h"
#include "chcore/container/list.h"
#include "chcore/container/rbtree.h"
#include "chcore/defs.h"
#include "chcore/type.h"
#include "stdbool.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "limits.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include "minunit.h"
#include "tmpfs_test.h"
#include <time.h>

#include "../../defs.h"

bool can_fail = false;

void *test_malloc(size_t size)
{
        if (can_fail && rand() % 3 == 0) {
                return NULL;
        } else {
                return malloc(size);
        }
}

#define malloc(size) test_malloc(size)

#define ALIGN      (sizeof(size_t) - 1)
#define ONES       ((size_t)-1 / UCHAR_MAX)
#define HIGHS      (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(x) ((x)-ONES & ~(x)&HIGHS)

size_t strlcpy(char *d, const char *s, size_t n)
{
        char *d0 = d;
        size_t *wd;

        if (!n--)
                goto finish;
        typedef size_t __attribute__((__may_alias__)) word;
        const word *ws;
        if (((uintptr_t)s & ALIGN) == ((uintptr_t)d & ALIGN)) {
                for (; ((uintptr_t)s & ALIGN) && n && (*d = *s); n--, s++, d++)
                        ;
                if (n && *s) {
                        wd = (void *)d;
                        ws = (const void *)s;
                        for (; n >= sizeof(size_t) && !HASZERO(*ws);
                             n -= sizeof(size_t), ws++, wd++)
                                *wd = *ws;
                        d = (void *)wd;
                        s = (const void *)ws;
                }
        }
        for (; n && (*d = *s); n--, s++, d++)
                ;
        *d = 0;
finish:
        return d - d0 + strlen(s);
}

#include "../../namei.c"
#include "../../internal_ops.c"

#define TEST_N     1000
#define TEST_LEVEL 10

/* inodes and dentries, declared globally for convenience */

struct inode *i_this, *i_thisis, *i_thisisvalid, *i_thisisvalidreg;
struct dentry *d_this, *d_thisis, *d_thisisvalid, *d_thisisvalidreg;

struct inode *i_this_sym0tovalid, *i_this_sym1tovalid;
struct dentry *d_this_sym0tovalid, *d_this_sym1tovalid;

struct inode *i_this_symloop, *i_this_toomanysym;
struct dentry *d_this_symloop, *d_this_toomanysym;

void init_root()
{
        tmpfs_root_dent = malloc(sizeof(struct dentry));

        tmpfs_root = tmpfs_inode_init(FS_DIR, 0);

        tmpfs_root_dent->inode = tmpfs_root;

        struct dentry *d_root_dot = tmpfs_root->d_ops->alloc_dentry();
        tmpfs_root->d_ops->add_dentry(tmpfs_root, d_root_dot, ".", 1);

        struct dentry *d_root_dotdot = tmpfs_root->d_ops->alloc_dentry();
        tmpfs_root->d_ops->add_dentry(tmpfs_root, d_root_dotdot, "..", 2);

        tmpfs_root->d_ops->link(tmpfs_root, d_root_dot, tmpfs_root);
        tmpfs_root->d_ops->link(tmpfs_root, d_root_dotdot, tmpfs_root);
}

/*
 * a wrapper used in test only
 * creating a directory under i_parent, with the name
 * omitting all the error handling
 */
static void test_mkdir(struct inode *i_parent, struct dentry *d_parent,
                       struct dentry **d_to_make, char *name, int len)
{
        *d_to_make = i_parent->d_ops->alloc_dentry();
        i_parent->d_ops->add_dentry(i_parent, *d_to_make, name, len);
        i_parent->d_ops->mkdir(i_parent, *d_to_make, 0);
}

/*
 * a wrapper used in test only
 * creating an inode of type(REG/SYM) under i_parent, with the name
 * omitting all the error handling
 */
static void test_mknod(struct inode *i_parent, struct dentry *d_parent,
                       struct dentry **d_to_make, int type, char *name,
                       size_t len)
{
        *d_to_make = i_parent->d_ops->alloc_dentry();
        i_parent->d_ops->add_dentry(i_parent, *d_to_make, name, len);
        i_parent->d_ops->mknod(i_parent, *d_to_make, 0, type);
}

/*
 * Functions that create test environment
 */

/*
 * Creating a path "/this/is/valid/reg",
 * where all prefixing components are directories,
 * and the last component is a regular file.
 */
static void this_is_valid_reg()
{
        /* creating /this */
        test_mkdir(tmpfs_root, tmpfs_root_dent, &d_this, "this", 4);
        i_this = d_this->inode;

        /* creating /this/is */
        test_mkdir(i_this, d_this, &d_thisis, "is", 2);
        i_thisis = d_thisis->inode;

        /* creating /this/is/valid */
        test_mkdir(i_thisis, d_thisis, &d_thisisvalid, "valid", 5);
        i_thisisvalid = d_thisisvalid->inode;

        /* creating /this/is/valid/reg */
        test_mknod(i_thisisvalid,
                   d_thisisvalid,
                   &d_thisisvalidreg,
                   FS_REG,
                   "reg",
                   3);
        i_thisisvalidreg = d_thisisvalidreg->inode;
}

/*
 * creating a path "/this/sym0tovalid"
 * where sym0tovalid is a symlink to "/this/is/valid"
 * the content of the symlink is an absolute path
 */
static void this_sym0tovalid_absolute()
{
        char *symlink = "////this///is////valid//";
        size_t len = strlen(symlink);
        test_mknod(
                i_this, d_this, &d_this_sym0tovalid, FS_SYM, "sym0tovalid", 11);
        i_this_sym0tovalid = d_this_sym0tovalid->inode;
        i_this_sym0tovalid->sym_ops->write_link(
                i_this_sym0tovalid, symlink, len);
}

/*
 * creating a path "/this/sym1tovalid"
 * where sym1tovalid is a symlink to "is/valid"
 * the content of the symlink is a relative path
 */
static void this_sym1tovalid_relative()
{
        char *symlink = "is////valid//";
        size_t len = strlen(symlink);
        test_mknod(
                i_this, d_this, &d_this_sym1tovalid, FS_SYM, "sym1tovalid", 11);
        i_this_sym1tovalid = d_this_sym1tovalid->inode;
        i_this_sym1tovalid->sym_ops->write_link(
                i_this_sym1tovalid, symlink, len);
}

/*
 * creating a path "/this/symloop"
 * where symloop is a symlink to itself, creating a loop
 */
static void this_symloop()
{
        char *symlink = "symloop";
        size_t len = strlen(symlink);

        test_mknod(i_this, d_this, &d_this_symloop, FS_SYM, symlink, len);
        i_this_symloop = d_this_symloop->inode;
        i_this_symloop->sym_ops->write_link(i_this_symloop, symlink, len);
}

/*
 * creating a path "/this/toomanysym"
 * where toomanysym is a symlink to "toomanysym/."
 * this will cause pathname lookup to use all reserved stack in nd->stack
 */
static void this_toomanysym()
{
        char *symlink = "toomanysym/.";
        size_t len = strlen(symlink);

        test_mknod(i_this,
                   d_this,
                   &d_this_toomanysym,
                   FS_SYM,
                   "toomanysym",
                   strlen("toomanysym"));
        i_this_toomanysym = d_this_toomanysym->inode;
        i_this_toomanysym->sym_ops->write_link(i_this_toomanysym, symlink, len);
}

static void init_test()
{
        init_root();

        this_is_valid_reg(); /* /this/is/valid/reg */

        this_sym0tovalid_absolute(); /* /this/sym0tovalid */

        this_sym1tovalid_relative(); /* /this/sym1tovalid */

        this_symloop(); /* /this/symloop */

        this_toomanysym();
}

#define LONGCOMP 300
MU_TEST(parentat_tests)
{
        struct nameidata nd;
        struct dentry *d_found;
        int err;

        /* a case that the component name is too long */
        char *long_comp = malloc(LONGCOMP);
        long_comp[0] = '/';
        for (int i = 1; i < LONGCOMP; i++) {
                long_comp[i] = 'a' + i % 26;
        }

        err = path_parentat(&nd, long_comp, 0, &d_found);
        mu_check(err == -ENAMETOOLONG);

        err = path_parentat(&nd, "/this/is/invalid/reg", 0, &d_found);
        mu_check(err == -ENOENT);

        /* a normal case that parentat() succeeds */
        err = path_parentat(&nd, "//this//is//valid///reg", 0, &d_found);
        mu_check(d_found->inode == i_thisisvalid);
        mu_check(err == 0);

        /* parentat() "/" */
        err = path_parentat(&nd, "/", 0, &d_found);
        mu_check(err == 0);
        mu_check(d_found->inode == tmpfs_root);

        /* there is a symlink loop */
        err = path_parentat(&nd, "//this////symloop///reg", 0, &d_found);
        mu_check(err == -ELOOP);

        /* too many symlinks to place on the stack */
        err = path_parentat(&nd, "//this///toomanysym//.//reg", 0, &d_found);
        mu_check(err == -ENOMEM);
}

MU_TEST(lookupat_tests)
{
        struct dentry *d_found;
        struct nameidata nd;
        int err;

        /*
         * A simple test case which would result in a null pointer
         * dereference in the previously buggy implementation
         */
        err = path_lookupat(&nd, "/", 0, &d_found);
        mu_check(err == 0);
        mu_check(d_found->inode == tmpfs_root);

        /* lookupat() with an invalid path */
        err = path_lookupat(&nd, "///this///is///invalid///reg", 0, &d_found);
        mu_check(err == -ENOENT);

        /* lookupat with a valid path */
        err = path_lookupat(&nd, "///this///is//valid//reg", 0, &d_found);
        mu_check(d_found->inode == i_thisisvalidreg);
        mu_check(err == 0);

        /* lookupat with dots in path */
        err = path_lookupat(
                &nd,
                "////this////is///..///.///is///.///valid///..///valid//reg",
                0,
                &d_found);
        mu_check(d_found->inode == i_thisisvalidreg);
        mu_check(err == 0);

        /*
         * lookupat() with a path contains symlink,
         * but explicitly set ND_NO_SYMLINK
         */
        err = path_lookupat(
                &nd, "/this/sym0tovalid/reg", ND_NO_SYMLINKS, &d_found);
        mu_check(err == -ELOOP);

        /* lookupat() with an absolute symlink */
        err = path_lookupat(&nd, "///this///sym0tovalid////reg", 0, &d_found);
        mu_check(err == 0);
        mu_check(d_found->inode == i_thisisvalidreg);

        /* lookupat() with a relative symlink */
        err = path_lookupat(&nd, "///this///sym1tovalid////reg", 0, &d_found);
        mu_check(err == 0);
        mu_check(d_found->inode == i_thisisvalidreg);

        /* trailing slashes cause lookupat() to target dirs */
        err = path_lookupat(&nd, "///this////is///valid///reg///", 0, &d_found);
        mu_check(err == -ENOTDIR);
}

MU_TEST(openat_tests)
{
        struct dentry *d_found;
        struct nameidata nd;
        int err;

        /*
         * A simple test case which would result in a null pointer
         * dereference in the previously buggy implementation
         */
        err = path_openat(&nd, "/", 0, 0, &d_found);
        mu_check(err == 0);
        mu_check(d_found->inode == tmpfs_root);

        /* open a file not exist */
        err = path_openat(&nd, "//this///is///valid///reg2", 0, 0, &d_found);
        mu_check(err == -ENOENT);

        /* open a file and all good */
        err = path_openat(&nd, "//this//is///valid///reg", 0, 0, &d_found);
        mu_check(err == 0);
        mu_check(d_found->inode == i_thisisvalidreg);

        /*
         * in all these open flags, namei.c cares about:
         * O_CREAT, O_NOFOLLOW, O_DIRECTORY
         * so test cases here test against these flags
         */

        /* O_CREAT set, but already exists */
        err = path_openat(&nd, "/this/is/valid/reg", O_CREAT, 0, &d_found);
        mu_check(err == 0);
        mu_check(d_found->inode == i_thisisvalidreg);

        /* O_CREAT set, but found a directory */
        err = path_openat(&nd, "/this/is/valid", O_CREAT, 0, &d_found);
        mu_check(err == -EISDIR);

        /* a failing creation */
        err = path_openat(
                &nd, "/this/is/valid/reg/thiswillfail", O_CREAT, 0, &d_found);

        mu_check(err == -ENOTDIR); /* fail because reg is not a dir */

        /* a successful creation */
        err = path_openat(&nd, "/this/is/valid/reg2", O_CREAT, 0, &d_found);
        mu_check(err == 0); /* creating a new file "reg2" */
        struct inode *i_reg2 = d_found->inode; /* created by path_openat() */

        err = path_lookupat(&nd, "/this/is/valid/reg2", 0, &d_found);
        mu_check(err == 0);
        mu_check(i_reg2 == d_found->inode); /* look it up, is the same inode */

        /*
         * Testing a path of a directory(trailing slashed) with O_CREAT.
         * we don't create a directory in open()
         */
        err = path_openat(
                &nd, "/this/is/valid//new_dir///", O_CREAT, 0, &d_found);
        mu_check(err == -EISDIR);

        /* when called with O_NOFOLLOW, openat() should return the symlink */
        err = path_openat(&nd, "/this/sym0tovalid", O_NOFOLLOW, 0, &d_found);
        mu_check(err == 0);
        mu_check(i_this_sym0tovalid == d_found->inode);

        /*
         * when called without O_NOFOLLOW, openat() should
         * return the inode which the symlink points to
         */
        err = path_openat(&nd, "/this/sym0tovalid", 0, 0, &d_found);
        mu_check(err == 0);
        mu_check(i_thisisvalid == d_found->inode);

        err = path_openat(
                &nd, "///this////is///valid////", O_DIRECTORY, 0, &d_found);
        mu_check(err == 0);
        mu_check(d_found->inode == i_thisisvalid);

        /* gives a regular file, asks for directory, fail */
        err = path_openat(
                &nd, "///this////is///valid////reg", O_DIRECTORY, 0, &d_found);
        mu_check(err == -ENOTDIR);

        /* opening a path ends with "." or ".." */
        err = path_openat(
                &nd, "/this///is//valid//.", O_DIRECTORY, 0, &d_found);
        mu_check(err == 0);
        mu_check(d_found->inode == i_thisisvalid);

        err = path_openat(
                &nd, "///this//is///valid///../", O_DIRECTORY, 0, &d_found);
        mu_check(err == 0);
        mu_check(d_found->inode == i_thisis);

        /* testing the cases that lookup_creat() might fail */
        for (int i = 0; i < 200; i++) {
                /* creating random files under "/" */
                int len = 15;
                char *name = malloc(len);
                name[0] = '/';
                for (int j = 1; j < len - 1; j++) {
                        name[j] = rand() % 26 + 'a';
                }
                name[len - 1] = '\0';

                can_fail = true;
                /* loop until succeed */
                while ((err = path_openat(&nd, name, O_CREAT, 0, &d_found))
                       != 0) {
                        mu_check(err == -ENOMEM);
                }
                can_fail = false;

                /* match the result with path_lookupat() */
                struct inode *i_openat = d_found->inode;
                err = path_lookupat(&nd, name, 0, &d_found);
                mu_check(err == 0);
                mu_check(i_openat == d_found->inode);

                free(name);
        }
}

/*
 * we create some test cases manually,
 * and check the outcome matches expectation
 */
MU_TEST_SUITE(namei_tests_manual)
{
        MU_RUN_TEST(parentat_tests);
        MU_RUN_TEST(lookupat_tests);
        MU_RUN_TEST(openat_tests);
}

MU_TEST_SUITE(namei_tests_fuzzing)
{
        /* TODO: use fuzzing to randomly test more cases */
}

int main()
{
        init_test();
        MU_RUN_SUITE(namei_tests_manual);
        MU_RUN_SUITE(namei_tests_fuzzing);
        MU_REPORT();
}