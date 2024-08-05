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

#ifndef TMPFS_DEFS_H
#define TMPFS_DEFS_H

#include <chcore/container/hashtable.h>
#include <chcore/container/radix.h>
#include <chcore/cpio.h>
#include <chcore/type.h>
#include <chcore/idman.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <stddef.h>
#include <string.h>

#define DEBUG 0

/*
 * To track tmpfs memory usage:
 * 1. Set DEBUG_MEM_USAGE to 1, and rebuild ChCore.
 *
 * 2. In ChCore shell, run tmpfs_mem.bin, which will start
 * recording **any** malloc()s and free()s in tmpfs afterwards.
 *
 * !!LIMITATION!!: memory usage in fs_base is **NOT** tracked,
 * and other modules like IPC will allocate/free memory on behalf of tmpfs,
 * causing mem tool to blame tmpfs for mem leakage, but this internal
 * tool won't help in these cases.
 *
 * 3. Run a program, for example fs_full_test.bin.
 * NOTE: the program should do proper cleaning up for its FS operations after
 * execution. If the program creates a file and does not delete it, it will
 * appear to be a memory leakage.
 *
 * 4. Run tmpfs_mem.bin again, and it will show the memory usage of
 * tmpfs ever since the first run of tmpfs_mem.bin
 *
 * 5. In normal cases (programs executed have cleaned up all the files and dirs
 * they created during execution), tmpfs_mem.bin will show nothing if no memory
 * leakage happened.
 * If the program actually created some files and dirs and did not
 * clean them up intentionally, then tmpfs_mem.bin show no leakage but may be
 * legal memory usage to creat and keep all these files.
 *
 * NOTE: Used only for debugging tmpfs, the memory tracking tool it self may
 * incur memory usage. Set to 0 when running tmpfs unit tests or compilation
 * will fail.
 */
#define DEBUG_MEM_USAGE 0

#define DENT_SIZE (1) /* A faked directory entry size */

#define MAX_PATH    (4096)
#define MAX_SYM_LEN MAX_PATH

#define MAX_NR_FID_RECORDS (1024)
#define MAX_DIR_HASH_BUCKETS (1024)

/* inode types */
#define FS_REG (8)
#define FS_DIR (4)
#define FS_SYM (10)

#define PREFIX         "[tmpfs]"
#define info(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#if 1
#define debug(fmt, ...) \
        printf(PREFIX "<%s:%d>: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define debug(fmt, ...) \
        do {            \
        } while (0)
#endif
#define warn(fmt, ...)  printf(PREFIX " " fmt, ##__VA_ARGS__)
#define error(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)

/* a simple string definition */
struct string {
        char *str;
        size_t len;
        u32 hash;
};

int init_string(struct string *s, const char *name, size_t len);
void free_string(struct string *s);

struct fid_record {
        struct inode *inode;
        u64 flags;
        off_t offset;
};

struct inode {
        bool opened;
        int type;
        int nlinks;
        off_t size;
        mode_t mode; /* not supported now */

        /* type-specific file content */
        union {
                char *symlink;
                struct radix data;
                struct htable dentries;
        };

        /* shared behaviour of all inodes */
        struct base_inode_ops *base_ops;

        /* type-specific operations */
        union {
                struct regfile_ops *f_ops;
                struct dir_ops *d_ops;
                struct symlink_ops *sym_ops;
        };

        /* other fields used by mmap */
        struct {
                bool valid;
                vaddr_t *array;
                size_t nr_used; /* Number of entries filled. */
                size_t size; /* Total capacity. */
                cap_t translated_pmo_cap; /* PMO cap of translated array. */
        } aarray;
};

struct dentry {
        struct string name;
        struct inode *inode;
        struct hlist_node node;
};

/* Internal operations of different types of inodes */

struct base_inode_ops {
        void (*open)(struct inode *inode);
        void (*close)(struct inode *inode);
        void (*dec_nlinks)(struct inode *inode);
        void (*inc_nlinks)(struct inode *inode);
        void (*free)(struct inode *inode);
        void (*stat)(struct inode *inode, struct stat *stat);
};

struct regfile_ops {
        ssize_t (*read)(struct inode *reg, char *buff, size_t len,
                        off_t offset);
        ssize_t (*write)(struct inode *reg, const char *buff, size_t len,
                         off_t offset);
        int (*truncate)(struct inode *reg, off_t len);
        int (*punch_hole)(struct inode *reg, off_t offset, off_t len);
        int (*collapse_range)(struct inode *reg, off_t offset, off_t len);
        int (*zero_range)(struct inode *reg, off_t offset, off_t len,
                          mode_t mode);
        int (*insert_range)(struct inode *reg, off_t offset, off_t len);
        int (*allocate)(struct inode *reg, off_t offset, off_t len,
                        int keep_size);
};

struct dir_ops {
        struct dentry *(*alloc_dentry)();
        void (*free_dentry)(struct dentry *dentry);
        int (*add_dentry)(struct inode *dir, struct dentry *dentry, char *name,
                          size_t len);
        void (*remove_dentry)(struct inode *dir, struct dentry *dentry);
        bool (*is_empty)(struct inode *dir);
        void (*link)(struct inode *dir, struct dentry *dentry,
                     struct inode *inode);
        void (*unlink)(struct inode *dir, struct dentry *dentry);
        int (*mkdir)(struct inode *dir, struct dentry *dentry, mode_t mode);
        int (*rmdir)(struct inode *dir, struct dentry *dentry);
        void (*rename)(struct inode *old_dir, struct dentry *old_dentry,
                       struct inode *new_dir, struct dentry *new_dentry);
        int (*mknod)(struct inode *dir, struct dentry *dentry, mode_t mode,
                     int type);
        struct dentry *(*dirlookup)(struct inode *dir, const char *name,
                                    size_t len);
        int (*scan)(struct inode *dir, unsigned int start, void *buf, void *end,
                    int *read_bytes);
};

struct symlink_ops {
        ssize_t (*read_link)(struct inode *symlink, char *buff, size_t len);
        ssize_t (*write_link)(struct inode *symlink, const char *buff,
                              size_t len);
};

/* internal_ops.c */
u64 hash_chars(const char *str, size_t len);
struct inode *tmpfs_inode_init(int type, mode_t mode);
void tmpfs_fs_stat(struct statfs *statbuf);

/* main.c */
void init_root(void);

/* tmpfs.c */
int tmpfs_mkdir(const char *path, mode_t mode);

extern struct inode *tmpfs_root;
extern struct dentry *tmpfs_root_dent;
extern struct id_manager fidman;
extern struct fid_record fid_records[MAX_NR_FID_RECORDS];

extern struct {
        struct cpio_file head;
        struct cpio_file *tail;
} g_files;

extern char __binary_ramdisk_cpio_start;

#if DEBUG_MEM_USAGE

#define DENTRY    0
#define STRING    1
#define INODE     2
#define SYMLINK   3
#define DATA_PAGE 4

/* 5 types of object will be created in tmpfs */
#define RECORD_SIZE 5

struct addr_to_size {
        vaddr_t addr;
        size_t size;
        struct list_head node;
        struct list_head order_list_node;
};

struct memory_use_msg {
        /* dentry, string, inode, symlink, data page */
        char *typename;
        struct list_head addr_to_size_head;
};

void tmpfs_get_mem_usage();
void tmpfs_revoke_mem_usage(void *addr, int type);
void tmpfs_record_mem_usage(void *addr, size_t size, int type);

#endif

#endif /* TMPFS_DEFS_H */