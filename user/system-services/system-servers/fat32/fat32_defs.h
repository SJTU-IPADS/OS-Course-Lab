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

#pragma once
#define D_NAME_MAX_LEN 256

#include <chcore-internal/fs_defs.h>

// TODO: copied from dirent.h because of DIR typedef conflict
struct dirent {
        ino_t d_ino;
        off_t d_off;
        unsigned short d_reclen;
        unsigned char d_type;
#define DT_DIR 4
#define DT_REG 8
        char d_name[D_NAME_MAX_LEN];
};

extern char sd[4];
extern FATFS *fs;
extern pthread_mutex_t fat_meta_lock;

/*
 * About FAT32 Inode Number
 * Inode Number(ino) should be unique for file AND dir
 * For files: we use the dentry location as ino
 * 	- Why no use first cluster number of block chain as ino:
 * 		Empty file has no block chain, so `sclust` field is meanless.
 * 	- FAT32 has no hard link, so there is no pair of paths points to one
 * file inode
 * 	- When open a path as FIL struct, we have `dir_sect` and `dir_ptr`
 * fields `dir_sect` stands the sect number of dentry placed `dir_ptr` is a
 * pointer in win[] buffer in FATFS struct so (`dir_sect` * SECT_SIZE +
 * (`dir_ptr` - win[])) is inode location in disk For direcories: we use the
 * first cluster number of block chain PLUS ONE as ino
 * 	- Each dir must have a corresponding block chain,
 * 		because it has '.' and '..' entries at least,
 * 		so `sclust` field is never meanless (being 0)
 * 	- Why PLUS ONE:
 * 		The first dentry in the dir and the dir chain beginning have
 * same location. But thanks to the alignment of dentry, file's ino is always
 * end with 0 We use the last bit to make a distinction between dir and file ino
 */

#define FAT_WINBASE              (uint64_t) fs->win
#define FP_DENTRY_WIN_OFFSET(fp) (uint64_t)(fp)->dir_ptr - FAT_WINBASE
#define FILE_INO(fp)                              \
        (ino_t)(((uint32_t)(fp)->dir_sect) << 10) \
                + (uint32_t)FP_DENTRY_WIN_OFFSET(fp)

#define DIR_INO(dp) (ino_t)(((dp)->obj.sclust << 10) + 1)

#define FAT_PATH_LEN_MAX (FS_REQ_PATH_LEN)
#define FAT_PATH_BUF_LEN (FAT_PATH_LEN_MAX + 1)