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

struct timespec __get_atime(const char *path, const char *_func, int _line);
struct timespec __get_mtime(const char *path, const char *_func, int _line);
struct timespec __get_ctime(const char *path, const char *_func, int _line);
ino_t __get_inode(const char *path, const char *_func, int _line);

void __fs_assert_noent(char *path, const char *_func, int _line);
void __fs_assert_reg_size(char *path, size_t size, const char *_func,
                          int _line);
void __fs_assert_is_dir(char *path, const char *_func, int _line);
void __fs_assert_is_reg(char *path, const char *_func, int _line);
void __fs_assert_is_symlink(char *path, const char *_func, int _line);
void __fs_check_mode(char *path, mode_t mode, const char *_func, int _line);
void __fs_check_nlinks(char *path, nlink_t nlink, const char *_func, int _line);
int __fs_assert_empty_buffer(char *buf, size_t size);
int __fs_assert_buffer_eq(char *buf1, char *buf2, size_t size);

#define FS_DO_NOTHING \
        do {          \
        } while (0)

/* Assert path points to no entry */
#define fs_assert_noent(path) __fs_assert_noent(path, __func__, __LINE__)
/* Assert path points to an file with size=size */
#define fs_assert_reg_size(path, size) \
        __fs_assert_reg_size(path, size, __func__, __LINE__)
/* Assert path points to a dir */
#define fs_assert_is_dir(path) __fs_assert_is_dir(path, __func__, __LINE__)
/* Assert path points to a regular file */
#define fs_assert_is_reg(path) __fs_assert_is_reg(path, __func__, __LINE__)
/* Assert path points to a symlink */
#define fs_assert_is_symlink(path) \
        __fs_assert_is_symlink(path, __func__, __LINE__)
/* Assert path points to an empty regular file */
#define fs_assert_empty_reg(path) fs_assert_reg_size(path, 0)
/* Assert first size bytes in buf is \0 */
#define fs_assert_empty_buffer(buf, size) \
        fs_test_eq(__fs_assert_empty_buffer(buf, size), 0)
/* Assert two buffer content are same */
#define fs_assert_buffer_eq(buf1, buf2, size) \
        fs_test_eq(__fs_assert_buffer_eq(buf1, buf2, size), 0);

#define get_inode(ino, path) ino = __get_inode(path, __func__, __LINE__)

#if CHECK_TIME
#define get_atime(t, path)        t = __get_atime(path, __func__, __LINE__)
#define get_mtime(t, path)        t = __get_mtime(path, __func__, __LINE__)
#define get_ctime(t, path)        t = __get_ctime(path, __func__, __LINE__)
#define fs_define_time(t)         struct timespec t
#define fs_assert_time_lt(t1, t2) fs_assert(t1.tv_sec < t2.tv_sec)
#define fs_assert_time_eq(t1, t2) fs_assert(t1.tv_sec == t2.tv_sec)
#else
#define get_atime(t, path)        FS_DO_NOTHING
#define get_mtime(t, path)        FS_DO_NOTHING
#define get_ctime(t, path)        FS_DO_NOTHING
#define fs_define_time(t)         FS_DO_NOTHING
#define fs_assert_time_lt(t1, t2) FS_DO_NOTHING
#define fs_assert_time_eq(t1, t2) FS_DO_NOTHING
#endif

#if CHECK_MODE
#define fs_check_mode(path, mode) \
        __fs_check_mode(path, mode, __func__, __LINE__)
#else
#define fs_check_mode(path, mode) FS_DO_NOTHING
#endif

#if CHECK_NLINK
#define fs_check_nlinks(path, nlink) \
        __fs_check_nlinks(path, nlink, __func__, __LINE__)
#else
#define fs_check_nlinks(path, nlink) FS_DO_NOTHING
#endif

#if CHECK_INODE_FIXED
#define fs_assert_inum_fixed(inum1, inum2) fs_test_eq(inum1, inum2)
#else
#define fs_assert_inum_fixed(inum1, inum2) \
        do {                               \
                printf("%d\n", inum1);     \
                printf("%d\n", inum2);     \
        } while (0)
#endif

#if !ENABLE_CHOWN
#define chown(p, s, t) 0
#endif

/* For porting */
#define fs_get_cwd() strdup(cwd)