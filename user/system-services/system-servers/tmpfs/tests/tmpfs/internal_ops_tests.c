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

#include "chcore/container/list.h"
#include "chcore/defs.h"
#include "chcore/error.h"
#include "chcore/idman.h"
#include "chcore/type.h"
#include "dirent.h"
#include "stdbool.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "limits.h"
#include "sys/stat.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include "minunit.h"
#include "tmpfs_test.h"
#include <time.h>

bool can_fail = false;

void *test_malloc(size_t size)
{
        if (can_fail && rand() % 5 == 0) {
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

#include "../../internal_ops.c"

struct inode *tmpfs_test_root = NULL;
struct dentry *tmpfs_test_root_dent = NULL;

/* check if memory is freed properly */
void *should_free[4]; /* in current test, an op will cause at most
                                      4 pointers in the fs to be freed*/
int free_idx, free_check = 0;

void init_test_root()
{
        tmpfs_test_root_dent = malloc(sizeof(struct dentry));

        tmpfs_test_root = tmpfs_inode_init(FS_DIR, 0);
        tmpfs_test_root_dent->inode = tmpfs_test_root;

        struct dentry *d_root_dot = tmpfs_test_root->d_ops->alloc_dentry();
        tmpfs_test_root->d_ops->add_dentry(tmpfs_test_root, d_root_dot, ".", 1);

        struct dentry *d_root_dotdot = tmpfs_test_root->d_ops->alloc_dentry();
        tmpfs_test_root->d_ops->add_dentry(
                tmpfs_test_root, d_root_dotdot, "..", 2);

        tmpfs_test_root->d_ops->link(
                tmpfs_test_root, d_root_dot, tmpfs_test_root);
        tmpfs_test_root->d_ops->link(
                tmpfs_test_root, d_root_dotdot, tmpfs_test_root);
}

/* Some simple wrappers for memory usage check */

static void tmpfs_inode_free_test(struct inode *inode)
{
        if (free_check) {
                mu_check(inode == should_free[free_idx--]);
        }
        tmpfs_inode_free(inode);
}

static void tmpfs_free_dent_test(struct dentry *dentry)
{
        if (free_check) {
                mu_check(dentry == should_free[free_idx--]);
        }
        tmpfs_free_dent(dentry);
}

#define MOD 6
long mkdir_counter = 0;

/*
 * rewrite for test
 * to better check the resource cleanup when failure happens.
 * code only changed to do some check,
 * the functionality stays unchanged.
 *
 * manually control the result of malloc and simulate failures.
 */
static int tmpfs_dir_mkdir_test_rewrite(struct inode *dir,
                                        struct dentry *dentry, mode_t mode)
{
#if DEBUG
        BUG_ON(dir->type != FS_DIR);
#endif

        int err;
        if (mkdir_counter % MOD == 0) {
                err = -ENOMEM;
        } else {
                err = dir->d_ops->mknod(dir, dentry, mode, FS_DIR);
        }

        if (err) {
                mkdir_counter++;
                return err;
        }

        struct inode *new_dir = dentry->inode;

        struct dentry *dot;
        if (mkdir_counter % MOD == 1) {
                dot = CHCORE_ERR_PTR(-ENOMEM);
        } else {
                dot = new_dir->d_ops->alloc_dentry();
        }

        if (CHCORE_IS_ERR(dot)) {
                err = CHCORE_PTR_ERR(dot);
                should_free[0] = new_dir, free_idx = 0;
                goto free_node;
        }

        if (mkdir_counter % MOD == 2) {
                err = -ENOMEM;
        } else {
                err = new_dir->d_ops->add_dentry(new_dir, dot, ".", 1);
        }

        if (err) {
                should_free[1] = dot, should_free[0] = new_dir, free_idx = 1;
                goto free_dot;
        }

        new_dir->d_ops->link(new_dir, dot, new_dir);

        struct dentry *dotdot;
        if (mkdir_counter % MOD == 3) {
                dotdot = CHCORE_ERR_PTR(-ENOMEM);
        } else {
                dotdot = new_dir->d_ops->alloc_dentry();
        }
        if (CHCORE_IS_ERR(dotdot)) {
                err = CHCORE_PTR_ERR(dotdot);
                should_free[1] = dot, should_free[0] = new_dir, free_idx = 1;
                goto remove_dot;
        }

        if (mkdir_counter % MOD == 4) {
                err = -ENOMEM;
        } else {
                err = new_dir->d_ops->add_dentry(new_dir, dotdot, "..", 2);
        }
        if (err) {
                should_free[2] = dotdot, should_free[1] = dot,
                should_free[0] = new_dir, free_idx = 2;
                goto free_dotdot;
        }

        /* mkdir_counter % MOD == 5, everything is OK! */

        new_dir->d_ops->link(new_dir, dotdot, dir);

        mkdir_counter++;
        return 0;

free_dotdot:
        new_dir->d_ops->free_dentry(dotdot);
remove_dot:
        new_dir->d_ops->remove_dentry(new_dir, dot);
free_dot:
        new_dir->d_ops->free_dentry(dot);
free_node:
        dentry->inode->base_ops->free(dentry->inode);

        dentry->inode = NULL;

        mkdir_counter++;
        return err;
}

static int tmpfs_dir_mkdir_test(struct inode *dir, struct dentry *dentry,
                                mode_t mode)
{
        int ret;

        /*
         * not doing free check for mkdir()
         * because different cases is hard to describe
         * using current free check semantic
         * for free check, we have a rewritten version of mkdir() to do this.
         */
        free_check = false;

        can_fail = true; /* malloc can fail, causing cleanup */
        ret = tmpfs_dir_mkdir(dir, dentry, mode);
        can_fail = false;

        free_check = true;

        return ret;
}

#define TEST_N 40

MU_TEST(fs_ops_tests)
{
        struct statfs *statfs = malloc(sizeof(struct statfs));
        tmpfs_fs_stat(statfs);
        const int faked_large_value = 1024 * 1024 * 512;
        mu_check(statfs->f_type == 0xcce7a9f5);
        mu_check(statfs->f_bsize == PAGE_SIZE);
        mu_check(statfs->f_blocks == faked_large_value);
        mu_check(statfs->f_bfree == faked_large_value);
        mu_check(statfs->f_bavail == faked_large_value);
        mu_check(statfs->f_files == faked_large_value);
        mu_check(statfs->f_ffree == faked_large_value);
        mu_check(statfs->f_namelen == MAX_PATH);
        mu_check(statfs->f_frsize == 512);
        mu_check(statfs->f_flags == 0);
}

MU_TEST(base_ops_tests)
{
        time_t t;
        int types[3] = {FS_REG, FS_DIR, FS_SYM};
        srand((unsigned int)time(&t));

        for (int i = 0; i < TEST_N; i++) {
                struct inode *inode;
                inode = tmpfs_inode_init(-1, 0); /* invalid type, fail */
                mu_check(inode == NULL);

                inode = tmpfs_inode_init(types[rand() % 3], 0);

                mu_check(!inode->opened);
                inode->base_ops->open(inode);
                mu_check(inode->opened);

                struct stat *stat = malloc(sizeof(struct stat));
                inode->base_ops->stat(inode, stat);
                mu_check(stat->st_size == inode->size);
                mu_check(stat->st_nlink == inode->nlinks);
                switch (inode->type) {
                case FS_REG:
                        mu_check(stat->st_mode == S_IFREG);
                        break;
                case FS_DIR:
                        mu_check(stat->st_mode == S_IFDIR);
                        break;
                case FS_SYM:
                        mu_check(stat->st_mode == S_IFLNK);
                        break;
                default:
                        BUG("Bad inode type");
                        break;
                }

                free_check = 1, free_idx = 0, should_free[0] = inode;
                inode->base_ops->close(inode); /* inode->nlinks == 0 */
                mu_check(free_idx == -1);
                free_check = 0;
        }
}

#define N_PAGES (35l)
#define SIZE    N_PAGES *PAGE_SIZE
static void init_reg_for_fallocate(struct inode *reg, char *file_content,
                                   char *input)
{
        ssize_t ret;
        /* some junk in the file */
        for (int i = 0; i < SIZE; i++) {
                input[i] = 'a' + rand() % 26;
        }

        ret = reg->f_ops->truncate(reg, SIZE);
        mu_check(ret == 0);
        ret = reg->f_ops->write(reg, input, SIZE, 0);
        mu_check(ret == SIZE);
        mu_check(reg->size == SIZE);

        memcpy(file_content, input, SIZE);
}

MU_TEST(reg_ops_tests)
{
        struct inode *reg = tmpfs_inode_init(FS_REG, 0);
        ssize_t ret;

        /* simulated file content */
        char *file_content = calloc(MAX_TEST_FILE_SIZE, 1);

        /* testing write() and read() */

        char *input = calloc(MAX_TEST_FILE_SIZE, 1);
        char *output = calloc(MAX_TEST_FILE_SIZE, 1);

        ssize_t write_len, write_off, read_len, read_off;

        for (int i = 0; i < TEST_N; i++) {
                memset(input, 0, MAX_TEST_FILE_SIZE);
                /* len + off < MAX_TEST_FILE_SIZE */
                write_len = rand() % 10 == 0 ? 0 : rand() % MAX_TEST_FILE_SIZE;
                write_off = rand() % (MAX_TEST_FILE_SIZE - write_len);

                for (int j = 0; j < write_len; j++) {
                        input[j] = (rand() % 26) + 'a';
                }

                size_t size, expected_size;
                size = reg->size;
                expected_size = size > write_len + write_off ?
                                        size :
                                        write_len + write_off;

                /* if len == 0, write does nothing */
                if (write_len == 0) {
                        expected_size = size;
                }

                ret = reg->f_ops->write(reg, input, write_len, write_off);
                mu_check(ret == write_len); /* the write will succeed */
                mu_check(reg->size == expected_size);

                /* simulated write */
                memcpy(file_content + write_off, input, write_len);

                for (int j = 0; j < TEST_N; j++) {
                        memset(output, 0, MAX_TEST_FILE_SIZE);
                        /* call read with len=0 every 5 times */
                        read_len = j % 5 == 0 ? 0 : rand() % MAX_TEST_FILE_SIZE;
                        read_off = rand() % MAX_TEST_FILE_SIZE;
                        ssize_t expected_ret;

                        if (read_off >= reg->size) {
                                /* offset out of bound */
                                expected_ret = 0;
                        } else {
                                /* offset in the file, at most read to EOF */
                                expected_ret = read_len + read_off
                                                               <= reg->size ?
                                                       read_len :
                                                       reg->size - read_off;
                        }

                        ret = reg->f_ops->read(reg, output, read_len, read_off);
                        mu_check(ret == expected_ret);

                        int cmp = memcmp(file_content + read_off, output, ret);
                        mu_check(cmp == 0);
                }
        }

        /* testing truncate() with read() */
        for (int i = 0; i < TEST_N; i++) {
                int type = i % 3;
                size_t len;

                switch (type) {
                case 0:
                        // clang-format off
                        reg->f_ops->truncate(reg, reg->size + rand() % (MAX_TEST_FILE_SIZE - reg->size));
                        // clang-format on
                        break;
                case 1:
                        len = reg->size == 0 ? 0 : rand() % reg->size;
                        reg->f_ops->truncate(reg, len);
                        memset(file_content + reg->size,
                               0,
                               MAX_TEST_FILE_SIZE - reg->size);
                        break;
                case 2:
                        reg->f_ops->truncate(reg, 0);
                        memset(file_content, 0, MAX_TEST_FILE_SIZE);
                        break;
                }

                for (int j = 0; j < TEST_N; j++) {
                        memset(output, 0, MAX_TEST_FILE_SIZE);
                        read_off = rand() % (reg->size + 1);
                        read_len = rand() % (MAX_TEST_FILE_SIZE - read_off);
                        ssize_t expected_ret;

                        if (read_off >= reg->size) {
                                expected_ret = 0;
                        } else {
                                expected_ret = read_len + read_off
                                                               <= reg->size ?
                                                       read_len :
                                                       reg->size - read_off;
                        }

                        ret = reg->f_ops->read(reg, output, read_len, read_off);
                        mu_check(ret == expected_ret);

                        int cmp = memcmp(file_content + read_off, output, ret);
                        mu_check(cmp == 0);
                }
        }

        /* testing fallocate() and associated functions */

        off_t off;
        off_t len;
        int keep_size;

        /* testing punch_hole() */

        init_reg_for_fallocate(reg, file_content, input);

        /* at this point there must be a page #22 */
        mu_check(radix_get(&reg->data, 22));

        off = SIZE / 2;
        len = SIZE / 4;

        ret = reg->f_ops->punch_hole(reg, off, len);
        mu_check(ret == 0);

        memset(file_content + off, 0, len);

        /* at this point, page #22 should be removed */
        mu_check(!radix_get(&reg->data, 22));

        ret = reg->f_ops->read(reg, output, SIZE, 0);
        mu_check(ret == reg->size);

        mu_check(!memcmp(file_content, output, SIZE));

        /* testing collapse_range() */

        init_reg_for_fallocate(reg, file_content, input);

        /* off + len out of range */
        off = SIZE;
        len = PAGE_SIZE;

        ret = reg->f_ops->collapse_range(reg, off, len);
        /*
         * Linux Manpage:
         * If the region specified by offset plus len reaches or passes the
         * end of file, an error is returned; instead, use ftruncate(2) to
         * truncate a file.
         */
        mu_check(ret == -EINVAL);

        off = PAGE_SIZE - 2;
        len = PAGE_SIZE - 2;

        ret = reg->f_ops->collapse_range(reg, off, len);
        /* granularity not match */
        mu_check(ret == -EINVAL);

        off = PAGE_SIZE * 3L;
        len = PAGE_SIZE * 4L;

        ret = reg->f_ops->collapse_range(reg, off, len);
        mu_check(ret == 0);
        mu_check(reg->size = SIZE - len);

        /* move all remaining content to file_content + off */
        memcpy(file_content + off, file_content + off + len, SIZE - off - len);

        ret = reg->f_ops->read(reg, output, reg->size, 0);
        mu_check(ret == reg->size);

        mu_check(!memcmp(file_content, output, reg->size));

        /* testing zero_range() */

        init_reg_for_fallocate(reg, file_content, input);

        off = SIZE / 2;
        len = SIZE;
        keep_size = 1; /* keep_size set */

        ret = reg->f_ops->zero_range(reg, off, len, keep_size);
        mu_check(ret == 0);

        /* space allocated but size not changed */
        mu_check(reg->size == SIZE);
        mu_check(radix_get(&reg->data, (off + len - 1) / PAGE_SIZE));

        memset(file_content + off, 0, len);
        ret = reg->f_ops->read(reg, output, len + off, 0);
        mu_check(ret == reg->size);
        mu_check(!memcmp(output, file_content, reg->size));

        init_reg_for_fallocate(reg, file_content, input);

        off = SIZE / 2;
        len = SIZE;
        keep_size = 0; /* keep_size not set */

        ret = reg->f_ops->zero_range(reg, off, len, keep_size);
        mu_check(ret == 0);

        /* space allocated and size changed */
        mu_check(reg->size == off + len);
        mu_check(radix_get(&reg->data, (off + len - 1) / PAGE_SIZE));

        memset(file_content + off, 0, len);
        ret = reg->f_ops->read(reg, output, len + off, 0);
        mu_check(ret == reg->size);
        mu_check(!memcmp(output, file_content, reg->size));

        /* testing insert_range() */

        init_reg_for_fallocate(reg, file_content, input);

        /* granularity not match */
        off = PAGE_SIZE + 3;
        len = PAGE_SIZE - 2;

        ret = reg->f_ops->insert_range(reg, off, len);
        mu_check(ret == -EINVAL);

        /* off out of range */
        off = SIZE;
        len = PAGE_SIZE;

        ret = reg->f_ops->insert_range(reg, off, len);
        mu_check(ret == -EINVAL);

        /* legal operation */
        off = PAGE_SIZE * 10L;
        len = PAGE_SIZE * 5L;

        ret = reg->f_ops->insert_range(reg, off, len);
        mu_check(ret == 0);
        mu_check(SIZE + len == reg->size);

        memcpy(file_content + off + len, file_content + off, reg->size - off);
        memset(file_content + off, 0, len);

        ret = reg->f_ops->read(reg, output, reg->size, 0);
        mu_check(ret == reg->size);
        mu_check(!memcmp(file_content, output, reg->size));

        /* testing fallocate() */

        init_reg_for_fallocate(reg, file_content, input);

        off = SIZE - PAGE_SIZE;
        len = 2l * PAGE_SIZE;
        keep_size = 1;

        ret = reg->f_ops->allocate(reg, len, off, keep_size);
        mu_check(ret == 0);

        /* size not changed but page allocated */
        mu_check(SIZE == reg->size);
        void *page;
        mu_check(page = radix_get(&reg->data, (SIZE + 1) / PAGE_SIZE));

        for (int i = 0; i < PAGE_SIZE; i++) {
                if (((char *)page)[i]) {
                        mu_assert(false, "new page not zeroed\n");
                        break;
                }
        }

        ret = reg->f_ops->read(reg, output, reg->size, 0);
        mu_check(ret == reg->size);

        mu_check(!memcmp(output, file_content, ret));

        /* keep_size not set */

        init_reg_for_fallocate(reg, file_content, input);

        off = SIZE - PAGE_SIZE;
        len = 2l * PAGE_SIZE;
        keep_size = 0;

        ret = reg->f_ops->allocate(reg, len, off, keep_size);
        mu_check(ret == 0);

        memset(file_content + SIZE, 0, off + len - SIZE);

        /* size changed and page allocated */
        mu_check(off + len == reg->size);
        mu_check(page = radix_get(&reg->data, (SIZE + 1) / PAGE_SIZE));

        for (int i = 0; i < PAGE_SIZE; i++) {
                if (((char *)page)[i]) {
                        mu_assert(false, "new page not zeroed\n");
                        break;
                }
        }

        ret = reg->f_ops->read(reg, output, reg->size, 0);
        mu_check(ret == reg->size);

        mu_check(!memcmp(output, file_content, ret));

        free(file_content);
        free(input);
        free(output);
        reg->base_ops->free(reg);
}

MU_TEST(dir_ops_tests)
{
        struct dentry *d_test;
        struct inode *i_test;
        int err;

        time_t t;
        srand((unsigned int)time(&t));

        /* we do the testing under root directory */
        init_test_root();
        free_check = 1;
        for (int i = 0; i < TEST_N * TEST_N * TEST_N; i++) {
                mu_check(tmpfs_test_root->nlinks == 2);
                mu_check(tmpfs_test_root->d_ops->is_empty(
                        tmpfs_test_root)); /* is_empty:
                                              true */

                /* testing alloc_dent() */
                d_test = tmpfs_test_root->d_ops->alloc_dentry();

                /*
                 * testing add_dentry()
                 */
                size_t size = tmpfs_test_root->size;
                mu_check(size == 2l * DENT_SIZE);

                struct dentry *d_found;

                /* creating a random test file name */
                int len = rand() % 10 + 1;
                char *test_name = malloc(len);
                for (int j = 0; j < len; j++) {
                        test_name[j] = rand() % 26 + 'a';
                }

                d_found = tmpfs_test_root->d_ops->dirlookup(
                        tmpfs_test_root, test_name, len);
                mu_check(d_found == NULL); /* not found before add_dentry */

                tmpfs_test_root->d_ops->add_dentry(
                        tmpfs_test_root, d_test, test_name, len);
                mu_check(tmpfs_test_root->size == size + DENT_SIZE);

                d_found = tmpfs_test_root->d_ops->dirlookup(
                        tmpfs_test_root, test_name, len);
                mu_check(d_found == d_test); /* found after add_dentry */
                mu_check(!tmpfs_test_root->d_ops->is_empty(
                        tmpfs_test_root)); /* is_empty:
                                              false*/

                /*
                 * testing mknod(), together with link()
                 */
                tmpfs_test_root->d_ops->mknod(
                        tmpfs_test_root, d_test, 0, FS_REG);
                i_test = d_test->inode;
                mu_check(i_test->type == FS_REG);
                mu_check(i_test->nlinks == 1);

                /*
                 * testing unlink()
                 */
                d_found = tmpfs_test_root->d_ops->dirlookup(
                        tmpfs_test_root, test_name, len);
                mu_check(d_found == d_test); /* found before unlink */

                /*
                 * unlinking root and test file
                 * causing its dentry and inode to be freed
                 *
                 * we use a reverse order to place the to-be-freed pointers
                 * and we check the pointer before calling free() against
                 * should_free[free_idx]
                 */
                should_free[1] = d_test, should_free[0] = i_test, free_idx = 1;
                tmpfs_test_root->d_ops->unlink(tmpfs_test_root, d_test);
                mu_check(free_idx == -1);

                d_found = tmpfs_test_root->d_ops->dirlookup(
                        tmpfs_test_root, test_name, len);
                mu_check(d_found == NULL); /* not found after unlink */
                mu_check(size == tmpfs_test_root->size);

                /*
                 * testing mkdir()
                 *
                 * reusing d_test, i_test and test_name,
                 * but creating a directory
                 */
                d_test = tmpfs_test_root->d_ops->alloc_dentry();

                tmpfs_test_root->d_ops->add_dentry(
                        tmpfs_test_root, d_test, test_name, len);

                while ((err = tmpfs_test_root->d_ops->mkdir(
                                tmpfs_test_root, d_test, 0))
                       != 0) {
                        mu_check(d_test->inode == NULL);
                        mu_check(free_idx == -1);
                }

                i_test = d_test->inode;
                mu_check(i_test != NULL);

                mu_check(tmpfs_test_root->nlinks == 3); /* ".", "..", and
                                                         * i_test's ".."
                                                         */

                mu_check(i_test->type == FS_DIR);
                mu_check(i_test->size == 2l * DENT_SIZE);
                mu_check(i_test->d_ops->is_empty(i_test));
                mu_check(i_test->nlinks == 2);

                struct dentry *dot = i_test->d_ops->dirlookup(i_test, ".", 1);
                struct dentry *dotdot =
                        i_test->d_ops->dirlookup(i_test, "..", 2);
                mu_check(dot->inode == i_test);
                mu_check(dotdot->inode == tmpfs_test_root);

                /* creating a file under /test, /test/tmpfile */
                struct dentry *d_tmpfile, *d_tmpfile_renamed;
                struct inode *i_tmpfile;

                len = rand() % 10 + 1;
                int renamed_len = len + rand() % 10 + 1; /* ensuring a different
                                                            name */

                char *tmpfile_name = malloc(len);
                for (int j = 0; j < len; j++) {
                        tmpfile_name[j] = rand() % 26 + 'a';
                }

                char *tmpfile_renamed_name = malloc(renamed_len);
                for (int j = 0; j < renamed_len; j++) {
                        tmpfile_renamed_name[j] = rand() % 26 + 'a';
                }

                d_tmpfile = i_test->d_ops->alloc_dentry();
                i_test->d_ops->add_dentry(i_test, d_tmpfile, tmpfile_name, len);

                i_test->d_ops->mknod(i_test, d_tmpfile, 0, FS_REG);
                i_tmpfile = d_tmpfile->inode;

                /* creating another dentry under /test, for rename() */
                d_tmpfile_renamed = i_test->d_ops->alloc_dentry();
                i_test->d_ops->add_dentry(i_test,
                                          d_tmpfile_renamed,
                                          tmpfile_renamed_name,
                                          renamed_len);

                /*
                 * testing rename() reg file
                 */
                struct dentry *d_tmpfile_found =
                        i_test->d_ops->dirlookup(i_test, tmpfile_name, len);
                struct dentry *d_tmpfile_renamed_found =
                        i_test->d_ops->dirlookup(
                                i_test, tmpfile_renamed_name, renamed_len);

                mu_check(d_tmpfile_found == d_tmpfile
                         && d_tmpfile_found->inode == i_tmpfile);
                mu_check(d_tmpfile_renamed_found == d_tmpfile_renamed
                         && d_tmpfile_renamed_found->inode == NULL);

                /* rename /test/tmpfile to /test/tmpfile_renamed */
                should_free[0] = d_tmpfile, free_idx = 0; /* d_tmpfile will be
                                                             freed */
                i_test->d_ops->rename(
                        i_test, d_tmpfile, i_test, d_tmpfile_renamed);
                mu_check(free_idx == -1);

                d_tmpfile_found =
                        i_test->d_ops->dirlookup(i_test, tmpfile_name, len);
                d_tmpfile_renamed_found = i_test->d_ops->dirlookup(
                        i_test, tmpfile_renamed_name, renamed_len);
                mu_check(d_tmpfile_found == NULL);
                mu_check(d_tmpfile_renamed_found == d_tmpfile_renamed
                         && d_tmpfile_renamed->inode == i_tmpfile);

                /*
                 * testing rmdir()
                 */
                /* not empty, should fail */
                mu_check(!i_test->d_ops->is_empty(i_test));
                mu_check(i_test->d_ops->dirlookup(
                                 i_test, tmpfile_renamed_name, renamed_len)
                         == d_tmpfile_renamed);
                mu_check(i_test->d_ops->rmdir(tmpfs_test_root, d_test)
                         == -ENOTEMPTY);

                should_free[1] = d_tmpfile_renamed,
                should_free[0] = d_tmpfile_renamed->inode, free_idx = 1;
                i_test->d_ops->unlink(i_test, d_tmpfile_renamed); /* /test is
                                                                     empty */
                mu_check(free_idx == -1);

                mu_check(i_test->d_ops->is_empty(i_test));
                mu_check(i_test->d_ops->dirlookup(
                                 i_test, tmpfile_renamed_name, renamed_len)
                         == NULL);

                should_free[3] = dot, should_free[2] = dotdot,
                should_free[1] = d_test, should_free[0] = i_test, free_idx = 3;
                mu_check(tmpfs_test_root->d_ops->rmdir(tmpfs_test_root, d_test)
                         == 0);
                mu_check(free_idx == -1);

                mu_check(tmpfs_test_root->d_ops->is_empty(
                        tmpfs_test_root)); /* / is empty */
                mu_check(tmpfs_test_root->nlinks == 2); /* only . and .. now */
                mu_check(tmpfs_test_root->size == 2l * DENT_SIZE);

                free(test_name);
                free(tmpfile_name);
                free(tmpfile_renamed_name);

                /*
                 * testing unlink() when opened, freed by close()
                 */
                struct dentry *d_tmp;
                struct inode *i_tmp;
                char *name = "testopenunlinkclose";
                d_tmp = tmpfs_test_root->d_ops->alloc_dentry();
                mu_check(d_tmp != NULL);
                err = tmpfs_test_root->d_ops->add_dentry(
                        tmpfs_test_root, d_tmp, name, strlen(name));
                mu_check(err == 0);
                err = tmpfs_test_root->d_ops->mknod(
                        tmpfs_test_root, d_tmp, 0, FS_REG);
                mu_check(err == 0);

                i_tmp = d_tmp->inode;
                i_tmp->base_ops->open(i_tmp);
                mu_check(i_tmp->opened);
                mu_check(i_tmp->nlinks == 1);

                /*
                 * with free_check set, inode_free() will fail
                 */
                should_free[0] = d_tmp, free_idx = 0;
                tmpfs_test_root->d_ops->unlink(tmpfs_test_root, d_tmp);
                mu_check(free_idx == -1);

                /*
                 * only when no one is using the file
                 * can the file be freed
                 */
                should_free[0] = i_tmp, free_idx = 0;
                i_tmp->base_ops->close(i_tmp);
                mu_check(free_idx == -1);

                /*
                 * creating /test/dir1/, and /test/dir1/file
                 * test renaming a directory, and dir_scan()
                 */

                /* creating /test */
                d_test = tmpfs_test_root->d_ops->alloc_dentry();
                tmpfs_test_root->d_ops->add_dentry(
                        tmpfs_test_root, d_test, "test", 4);

                /* since mkdir can fail in the test.. */
                while (tmpfs_test_root->d_ops->mkdir(
                        tmpfs_test_root, d_test, 123)) {
                        ;
                }

                mu_check(tmpfs_test_root->nlinks == 3);

                i_test = d_test->inode;
                mu_check(i_test->nlinks == 2);

                /* creating /test/dir1 */
                struct dentry *d_dir1 = i_test->d_ops->alloc_dentry();
                i_test->d_ops->add_dentry(i_test, d_dir1, "dir1", 4);
                while (i_test->d_ops->mkdir(i_test, d_dir1, 123)) {
                        ;
                }
                mu_check(i_test->nlinks == 3);

                struct inode *i_dir1 = d_dir1->inode;
                mu_check(i_dir1->nlinks == 2);

                /* creating /test/dir1/file */
                struct dentry *d_file = i_dir1->d_ops->alloc_dentry();
                i_dir1->d_ops->add_dentry(i_dir1, d_file, "file", 4);
                i_dir1->d_ops->mknod(i_dir1, d_file, 0, FS_REG);

                mu_check(i_dir1->nlinks == 2);

                struct inode *i_file = d_file->inode;
                mu_check(i_file->nlinks == 1);

                /* testing dir_scan() */
                void *buf, *end;
                struct dirent *dirents;
                int read_bytes, cnt;

                dirents = malloc(sizeof(struct dirent) * 3);
                buf = (void *)dirents;
                /* a little over 2 dirents, so third fill_dirent() will fail */
                end = buf + 2 * sizeof(struct dirent) + 1;

                cnt = i_dir1->d_ops->scan(i_dir1, 0, buf, end, &read_bytes);
                mu_check(cnt == 2);
                mu_check(read_bytes == 2 * sizeof(struct dirent));

                mu_check(dirents[0].d_type == FS_DIR);
                mu_check(!strcmp(dirents[0].d_name, "."));
                mu_check(dirents[1].d_type == FS_REG);
                mu_check(!strcmp(dirents[1].d_name, "file"));

                end = buf + 4 * sizeof(struct dirent);
                cnt = i_dir1->d_ops->scan(i_dir1, 0, buf, end, &read_bytes);
                mu_check(cnt == 3);
                mu_check(read_bytes == 3 * sizeof(struct dirent));

                /* all dentries have been read */
                mu_check(dirents[0].d_type == FS_DIR);
                mu_check(!strcmp(dirents[0].d_name, "."));
                mu_check(dirents[1].d_type == FS_REG);
                mu_check(!strcmp(dirents[1].d_name, "file"));
                mu_check(dirents[2].d_type == FS_DIR);
                mu_check(!strcmp(dirents[2].d_name, ".."));

                /* testing rename() a dir */

                /* rename /test/dir1 -> /test/dir2/renamed */

                /* creating /test/dir2 */
                struct dentry *d_dir2 = i_test->d_ops->alloc_dentry();
                i_test->d_ops->add_dentry(i_test, d_dir2, "dir2", 4);
                while (i_test->d_ops->mkdir(i_test, d_dir2, 123))
                        ;
                mu_check(i_test->nlinks == 4);

                struct inode *i_dir2 = d_dir2->inode;
                mu_check(i_dir2->nlinks == 2);

                /* create a dentry for rename , not allocate inode in it */
                struct dentry *d_renamed = i_dir2->d_ops->alloc_dentry();
                i_dir2->d_ops->add_dentry(i_dir2, d_renamed, "renamed", 7);

                mu_check(i_dir2->nlinks == 2);

                /* /test/dir1 -> /test/dir2/renamed */
                should_free[0] = d_dir1, free_idx = 0;
                i_test->d_ops->rename(i_test, d_dir1, i_dir2, d_renamed);
                mu_check(free_idx == -1); /* /test/dir1 dentry freed */

                struct inode *i_renamed = d_renamed->inode;
                mu_check(d_renamed->inode == i_dir1); /* inode renamed */
                mu_check(i_test->nlinks == 3); /* dir1 link removed */
                mu_check(i_dir2->nlinks == 3);

                struct dentry *renamed_dot =
                        i_renamed->d_ops->dirlookup(i_renamed, ".", 1);
                mu_check(renamed_dot->inode == i_renamed);
                struct dentry *renamed_dotdot =
                        i_renamed->d_ops->dirlookup(i_renamed, "..", 2);
                mu_check(renamed_dotdot->inode = i_dir2); /* dotdot changed! */

                struct dentry *renamed_file =
                        i_renamed->d_ops->dirlookup(i_renamed, "file", 4);
                mu_check(renamed_file == d_file);
                mu_check(renamed_file->inode == i_file);

                /* cleaning up */

                should_free[1] = d_file, should_free[0] = i_file, free_idx = 1;
                i_renamed->d_ops->unlink(i_renamed, d_file);
                mu_check(free_idx == -1);

                should_free[3] = renamed_dot, should_free[2] = renamed_dotdot,
                should_free[1] = d_renamed, should_free[0] = i_renamed,
                free_idx = 3;
                err = i_dir2->d_ops->rmdir(i_dir2, d_renamed);
                mu_check(!err);
                mu_check(free_idx == -1);

                struct dentry *dir2_dot =
                        i_dir2->d_ops->dirlookup(i_dir2, ".", 1);
                struct dentry *dir2_dotdot =
                        i_dir2->d_ops->dirlookup(i_dir2, "..", 2);

                should_free[3] = dir2_dot, should_free[2] = dir2_dotdot,
                should_free[1] = d_dir2, should_free[0] = i_dir2, free_idx = 3;
                err = i_test->d_ops->rmdir(i_test, d_dir2);
                mu_check(!err);
                mu_check(free_idx == -1);

                struct dentry *test_dot =
                        i_test->d_ops->dirlookup(i_test, ".", 1);
                struct dentry *test_dotdot =
                        i_test->d_ops->dirlookup(i_test, "..", 2);

                should_free[3] = test_dot, should_free[2] = test_dotdot,
                should_free[1] = d_test, should_free[0] = i_test, free_idx = 3;
                err = tmpfs_test_root->d_ops->rmdir(tmpfs_test_root, d_test);
                mu_check(!err);
                mu_check(free_idx == -1);
        }
        free_check = 0;
        /* end of tests */

        /*
         * cleaning up
         */
        struct dentry *root_dot =
                tmpfs_test_root->d_ops->dirlookup(tmpfs_test_root, ".", 1);
        struct dentry *root_dotdot =
                tmpfs_test_root->d_ops->dirlookup(tmpfs_test_root, "..", 2);
        tmpfs_test_root->d_ops->unlink(tmpfs_test_root, root_dot);
        tmpfs_test_root->d_ops->unlink(tmpfs_test_root, root_dotdot);

        free(tmpfs_test_root_dent);
        tmpfs_test_root = NULL;
        tmpfs_test_root_dent = NULL;
}

MU_TEST(symlink_ops_tests)
{
        time_t t;
        ssize_t ret;

        srand((unsigned)(time(&t)));

        struct inode *symlink = tmpfs_inode_init(FS_SYM, 0);
        char *input = malloc(MAX_SYM_LEN + 2048);
        char *output = malloc(MAX_SYM_LEN + 2048);

        for (int i = 0; i < TEST_N * TEST_N; i++) {
                /*
                 * chances are len > MAX_SYM_LEN,
                 * so that writelink should return -ENAMETOOLONG
                 */
                int len = rand() % (MAX_SYM_LEN + 2048);
                for (int j = 0; j < len; j++) {
                        input[i] = rand() % 26 + 'a';
                }

                ret = symlink->sym_ops->write_link(symlink, input, len);
                if (len > MAX_SYM_LEN) {
                        len = -ENAMETOOLONG;
                }

                mu_check(ret == len);
                if (ret == -ENAMETOOLONG) {
                        memset(input, 0, MAX_SYM_LEN);
                        continue;
                }
                mu_check(memcmp(symlink->symlink, input, len) == 0);

                ret = symlink->sym_ops->read_link(symlink, output, len);
                mu_check(ret == len);
                mu_check(memcmp(symlink->symlink, output, len) == 0);
                memset(input, 0, MAX_SYM_LEN);
                memset(output, 0, MAX_SYM_LEN);
        }

        free(input);
        free(output);
        symlink->base_ops->free(symlink);
}

MU_TEST_SUITE(internal_ops_tests)
{
        MU_RUN_TEST(dir_ops_tests);
        MU_RUN_TEST(fs_ops_tests);
        MU_RUN_TEST(base_ops_tests);
        MU_RUN_TEST(reg_ops_tests);
        MU_RUN_TEST(symlink_ops_tests);
}

int main(void)
{
        base_inode_ops.free = tmpfs_inode_free_test;
        dir_ops.free_dentry = tmpfs_free_dent_test;

        /* test with different mkdir implementation */

        dir_ops.mkdir = tmpfs_dir_mkdir_test;
        MU_RUN_SUITE(internal_ops_tests);

        dir_ops.mkdir = tmpfs_dir_mkdir_test_rewrite;
        MU_RUN_SUITE(internal_ops_tests);

        MU_REPORT();
        return 0;
}