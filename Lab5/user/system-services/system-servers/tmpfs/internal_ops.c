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
 * This file contains some internal operations
 * that are closely related to the management of
 * some core data structures.
 *
 * These functions are mostly called using function pointers(with rare
 * exceptions), by the form of "methods" in OOP languages. Most of these
 * functions *are not* meant to be called directly!
 *
 * We have grouped these operations into four catagories
 * 1. base inode operations 2. regular file operations
 * 3. directory operations  4. symlink operations
 */

#include "chcore/container/hashtable.h"
#include "chcore/container/list.h"
#include "dirent.h"
#include "sys/stat.h"
#include <chcore/error.h>
#include <chcore/falloc.h>
#include <chcore/container/radix.h>
#include "defs.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

struct inode *tmpfs_root = NULL;
struct dentry *tmpfs_root_dent = NULL;

struct base_inode_ops base_inode_ops;
struct regfile_ops regfile_ops;
struct dir_ops dir_ops;
struct symlink_ops symlink_ops;

#if DEBUG_MEM_USAGE
void debug_radix_free(void *page)
{
        tmpfs_revoke_mem_usage(page, DATA_PAGE);
        free(page);
}
#endif

/* string utils */
u64 hash_chars(const char *str, size_t len)
{
        u64 seed = 131; /* 31 131 1313 13131 131313 etc.. */
        u64 hash = 0;
        int i;

        if (len < 0) {
                while (*str) {
                        hash = (hash * seed) + *str;
                        str++;
                }
        } else {
                for (i = 0; i < len; ++i)
                        hash = (hash * seed) + str[i];
        }

        return hash;
}

void hash_string(struct string *s)
{
        s->hash = hash_chars(s->str, s->len);
}

int init_string(struct string *s, const char *name, size_t len)
{
        free_string(s);

        /*
         * s->str is allocated and copied,
         * remember to free it afterwards
         */
        s->str = malloc(len + 1);
        if (!s->str) {
                return -ENOMEM;
        }

#if DEBUG_MEM_USAGE
        tmpfs_record_mem_usage(s->str, len + 1, STRING);
#endif

        memcpy(s->str, name, len);
        s->str[len] = '\0';

        s->len = len;

        hash_string(s);
        return 0;
}

void free_string(struct string *s)
{
        if (s->str) {
#if DEBUG_MEM_USAGE
                tmpfs_revoke_mem_usage(s->str, STRING);
#endif
                free(s->str);
        }
        s->str = NULL;
}

/* FS operations */

/**
 * @brief Get the fs's metadata.
 * @return statbuf A pointer to the caller provided buff, filled with
 * preset metadata of tmpfs.
 */
void tmpfs_fs_stat(struct statfs *statbuf)
{
        const int faked_large_value = 1024 * 1024 * 512;
        /* Type of filesystem (see below): ChCorE7mpf5 */
        statbuf->f_type = 0xCCE7A9F5;
        /* Optimal transfer block size */
        statbuf->f_bsize = PAGE_SIZE;
        /* Total data blocks in filesystem */
        statbuf->f_blocks = faked_large_value;
        /* Free blocks in filesystem */
        statbuf->f_bfree = faked_large_value;
        /* Free blocks available to unprivileged user */
        statbuf->f_bavail = faked_large_value;
        /* Total file nodes in filesystem */
        statbuf->f_files = faked_large_value;
        /* Free file nodes in filesystem */
        statbuf->f_ffree = faked_large_value;
        /* Filesystem ID (See man page) */
        memset(&statbuf->f_fsid, 0, sizeof(statbuf->f_fsid));
        /* Maximum length of filenames */
        statbuf->f_namelen = MAX_PATH;
        /* Fragment size (since Linux 2.6) */
        statbuf->f_frsize = 512;
        /* Mount flags of filesystem (since Linux 2.6.36) */
        statbuf->f_flags = 0;
}

/* Base inode operations */

/**
 * @brief Create an empty inode of a particular type.
 * @param type The type of the inode to be created,
 * can be one of FS_REG, FS_DIR, FS_SYM.
 * @param mode The access mode of the file, ignored for now.
 * @return inode The newly created inode, NULL if type is illegal or out of
 * memory.
 * @note The created inode is not ready for use. For example the directory is
 * empty and has no dot and dotdot dentries.
 */
struct inode *tmpfs_inode_init(int type, mode_t mode)
{
        struct inode *inode = malloc(sizeof(struct inode));

        if (inode == NULL) {
                return inode;
        }

#if DEBUG_MEM_USAGE
        tmpfs_record_mem_usage(inode, sizeof(struct inode), INODE);
#endif

        /* Type-specific fields initialization */
        switch (type) {
        case FS_REG:
                inode->f_ops = &regfile_ops;

#if DEBUG_MEM_USAGE
                init_radix_w_deleter(&inode->data, debug_radix_free);
#else
                init_radix_w_deleter(&inode->data, free);
#endif
                break;
        case FS_DIR:
                inode->d_ops = &dir_ops;
                init_htable(&inode->dentries, MAX_DIR_HASH_BUCKETS);
                break;
        case FS_SYM:
                inode->sym_ops = &symlink_ops;
                inode->symlink = NULL;
                break;
        default:
#if DEBUG_MEM_USAGE
                tmpfs_revoke_mem_usage(inode, INODE);
#endif

                free(inode);
                return NULL;
        }

        inode->type = type;
        inode->nlinks = 0; /* ZERO links now */
        inode->size = 0;
        inode->mode = mode;
        inode->opened = false;
        inode->base_ops = &base_inode_ops;

        return inode;
}

/**
 * @brief Open a file.
 * @param inode Inode to be opened.
 * @note Tmpfs does not have anything like an openfile table, and does not to do
 * reference counting on the opened files, they are all done in fs_base. The
 * only thing we have to worry is that it is completely legal for a file to be
 * unlinked when someone is still holding the open file handle and using it, and
 * we can not free the inode at that time, so a flag indicates the file is
 * opened is needed.
 */
static inline void tmpfs_inode_open(struct inode *inode)
{
        inode->opened = true;
}

/**
 * @brief Close a file. If the file has no link, free it.
 * @param inode Inode to be closed
 * @note As said above, when closing, the file may be unlinked before, we should
 * clean it up then.
 */
static void tmpfs_inode_close(struct inode *inode)
{
#if DEBUG
        BUG_ON(!inode->opened);
        BUG_ON(inode->nlinks < 0);
#endif

        inode->opened = false;

        if (inode->nlinks == 0) {
                inode->base_ops->free(inode);
        }
}

/**
 * @brief Increase the inode's nlinks by 1.
 * @param inode
 */
static inline void tmpfs_inode_inc_nlinks(struct inode *inode)
{
#if DEBUG
        BUG_ON(inode->nlinks < 0);
#endif
        inode->nlinks++;
}

/**
 * @brief Decrease the inode's nlinks by 1. If nlinks reaches
 * zero and the inode is not being used, free it.
 * @param inode
 */
static void tmpfs_inode_dec_nlinks(struct inode *inode)
{
#if DEBUG
        BUG_ON(inode->nlinks <= 0);
#endif

        inode->nlinks--;
        if (inode->nlinks == 0 && !inode->opened) {
                inode->base_ops->free(inode);
        }
}

/**
 * @brief Free an inode.
 * @param inode Inode to be freed.
 */
static void tmpfs_inode_free(struct inode *inode)
{
        /* freeing type-specific fields */
        switch (inode->type) {
        case FS_REG:
                radix_free(&inode->data);
                break;
        case FS_DIR:
                htable_free(&inode->dentries);
                break;
        case FS_SYM:
                if (inode->symlink) {
#if DEBUG_MEM_USAGE
                        tmpfs_revoke_mem_usage(inode->symlink, SYMLINK);
#endif
                        free(inode->symlink);
                }
                break;
        default:
                BUG_ON(1);
        }

#if DEBUG_MEM_USAGE
        tmpfs_revoke_mem_usage(inode, INODE);
#endif
        free(inode);
}

/**
 * @brief Get an inode's metadata.
 * @param inode The inode to stat.
 * @return stat The pointer of the buffer to fill the stats in.
 */
static void tmpfs_inode_stat(struct inode *inode, struct stat *stat)
{
        memset(stat, 0, sizeof(struct stat));

        /* We currently support only a small part of stat fields */
        switch (inode->type) {
        case FS_DIR:
                stat->st_mode = S_IFDIR;
                break;
        case FS_REG:
                stat->st_mode = S_IFREG;
                break;
        case FS_SYM:
                stat->st_mode = S_IFLNK;
                break;
        default:
                BUG_ON(1);
        }

#if DEBUG
        BUG_ON(inode->size >= LONG_MAX);
#endif

        stat->st_size = (off_t)inode->size;
        stat->st_nlink = inode->nlinks;
        stat->st_ino = (ino_t)(uintptr_t)inode;
}

/* Regular file operations */

/**
 * @brief read a file's content at offset, and read for size bytes.
 * @param reg The file to be read.
 * @param size Read at most size bytes.
 * @param offset The starting offset of the read.
 * @param buff The caller provided buffer to be filled with the file content.
 * @return ssize_t The actual number of bytes that have been read into the
 * buffer.
 */
static ssize_t tmpfs_file_read(struct inode *reg, char *buff, size_t size,
                               off_t offset)
{
#if DEBUG
        BUG_ON(reg->type != FS_REG);
#endif

        u64 page_no, page_off;
        u64 cur_off = offset;
        size_t to_read;
        void *page;

        /* Returns 0 according to man pages. */
        if (offset >= reg->size)
                return 0;

        size = MIN(reg->size - offset, size);

        while (size > 0 && cur_off <= reg->size) {
                page_no = cur_off / PAGE_SIZE;
                page_off = cur_off % PAGE_SIZE;

                page = radix_get(&reg->data, page_no);
                to_read = MIN(size, PAGE_SIZE - page_off);
                if (!page)
                        memset(buff, 0, to_read);
                else
                        memcpy(buff, page + page_off, to_read);
                cur_off += to_read;
                buff += to_read;
                size -= to_read;
        }

        return (ssize_t)(cur_off - offset);
}

/**
 * @brief Write a file's content at offset, and write for size bytes.
 * @param reg The file to be written.
 * @param buff The caller provided buffer that is to be written into the file.
 * @param size Write at most size bytes.
 * @param offset The starting offset of the write.
 * @return ssize_t The actual number of bytes that have been written.
 */
static ssize_t tmpfs_file_write(struct inode *reg, const char *buff, size_t len,
                                off_t offset)
{
#if DEBUG
        BUG_ON(reg->type != FS_REG);
#endif

        u64 page_no, page_off;
        off_t cur_off = offset;
        int to_write;
        void *page = NULL;

        if (len == 0)
                return 0;

        while (len > 0) {
                page_no = cur_off / PAGE_SIZE;
                page_off = cur_off % PAGE_SIZE;

                page = radix_get(&reg->data, page_no);
                if (!page) {
                        page = aligned_alloc(PAGE_SIZE, PAGE_SIZE);
                        if (!page)
                                return (ssize_t)(cur_off - offset);

#if DEBUG_MEM_USAGE
                        tmpfs_record_mem_usage(page, PAGE_SIZE, DATA_PAGE);
#endif

                        if (page_off)
                                memset(page, 0, page_off);
                        radix_add(&reg->data, page_no, page);
                }

#if DEBUG
                BUG_ON(page == NULL);
#endif

                to_write = MIN(len, PAGE_SIZE - page_off);
                memcpy(page + page_off, buff, to_write);
                cur_off += to_write;
                buff += to_write;
                len -= to_write;
        }

        if (cur_off > reg->size) {
                reg->size = cur_off;
                if (cur_off % PAGE_SIZE && page) {
                        /* if the last write cannot fill the last page, set the
                         * remaining space to zero to ensure the correctness of
                         * the file_read */
                        page_off = cur_off % PAGE_SIZE;
                        memset(page + page_off, 0, PAGE_SIZE - page_off);
                }
        }
        return (ssize_t)(cur_off - offset);
}

/**
 * @brief Change a file's size to a length.
 * @param reg The file to truncate.
 * @param len The length to change to.
 * @return 0 on success.
 * @note When increasing the file's size, we only allocate the memory space in a
 * lazy fashion. If do need the space to be allocated, consider functions
 * related to fallocate().
 */
static int tmpfs_file_truncate(struct inode *reg, off_t len)
{
#if DEBUG
        BUG_ON(reg->type != FS_REG);
#endif

        u64 page_no, page_off;
        void *page;
        if (len == 0) {
                /* free radix tree and init an empty one */
                radix_free(&reg->data);

#if DEBUG_MEM_USAGE
                init_radix_w_deleter(&reg->data, debug_radix_free);
#else
                init_radix_w_deleter(&reg->data, free);
#endif
                reg->size = 0;
        } else if (len > reg->size) {
                /* truncate should not allocate the space for the file */
                reg->size = len;
        } else if (len < reg->size) {
                size_t cur_off = len;
                size_t to_write;
                page_no = cur_off / PAGE_SIZE;
                page_off = cur_off % PAGE_SIZE;
                if (page_off) {
                        /*
                         * if the last write cannot fill the last page, set the
                         * remaining space to zero to ensure the correctness of
                         * the file_read
                         */
                        page = radix_get(&reg->data, page_no);
                        if (page) {
                                to_write = MIN(reg->size - len,
                                               PAGE_SIZE - page_off);
                                memset(page + page_off, 0, to_write);
                                cur_off += to_write;
                        }
                }
                while (cur_off < reg->size) {
                        page_no = cur_off / PAGE_SIZE;
                        radix_del(&reg->data, page_no, 1);
                        cur_off += PAGE_SIZE;
                }
                reg->size = len;
        }

        return 0;
}

/**
 * @brief Deallocate disk space(memory in tmpfs) in the range specified by
 * params offset to offset + len for a file.
 * @param reg The file to modify.
 * @param offset The start of the deallocate range.
 * @param len The length of the deallocate range.
 * @return int 0 on success, -1 if radix_del() fails.
 * @note For full pages in the range, they are deleted from the file. For
 * partial pages in the range, they are set to zero.
 */
static int tmpfs_file_punch_hole(struct inode *reg, off_t offset, off_t len)
{
#if DEBUG
        BUG_ON(reg->type != FS_REG);
#endif

        u64 page_no, page_off;
        u64 cur_off = offset;
        off_t to_remove;
        void *page;
        int err;

        while (len > 0) {
                page_no = cur_off / PAGE_SIZE;
                page_off = cur_off % PAGE_SIZE;

                to_remove = MIN(len, PAGE_SIZE - page_off);
                cur_off += to_remove;
                len -= to_remove;
                page = radix_get(&reg->data, page_no);
                if (page) {
                        /*
                         * Linux Manpage:
                         * Within the specified range, partial filesystem blocks
                         * are zeroed, and whole filesystem blocks are removed
                         * from the file.
                         */
                        if (to_remove == PAGE_SIZE || cur_off == reg->size) {
                                err = radix_del(&reg->data, page_no, 1);
                                /* if no err, just continue! */
                                if (err) {
                                        return err;
                                }
                        } else {
                                memset(page + page_off, 0, to_remove);
                        }
                }
        }
        return 0;
}

/**
 * @brief Collapse file space in the range from offset to offset + len.
 * @param reg The file to be collapsed.
 * @param offset The start of the to be collapsed range.
 * @param len The length of the range.
 * @return int 0 on success, -EINVAL if the granularity not match or the params
 are illegal, or -1 if radix_del() fails, or -ENOMEM if radix_add() fails.
 * @note The range will be *removed*, without leaving a hole in the file. That
 is, the content start at offset + len will be at offset when this is done, and
 the file size will be len bytes smaller. Tmpfs only allows this to be done at a
 page granularity.
 */
static int tmpfs_file_collapse_range(struct inode *reg, off_t offset, off_t len)
{
#if DEBUG
        BUG_ON(reg->type != FS_REG);
#endif

        u64 page_no1, page_no2;
        u64 cur_off = offset;
        void *page1;
        void *page2;
        u64 remain;
        int err;
        off_t dist;

        /* To ensure efficient implementation, offset and len must be a mutiple
         * of the filesystem logical block size */
        if (offset % PAGE_SIZE || len % PAGE_SIZE)
                return -EINVAL;
        if (offset + len >= reg->size)
                return -EINVAL;

        remain = ((reg->size + PAGE_SIZE - 1) - (offset + len)) / PAGE_SIZE;
        dist = len / PAGE_SIZE;
        while (remain-- > 0) {
                page_no1 = cur_off / PAGE_SIZE;
                page_no2 = page_no1 + dist;

                cur_off += PAGE_SIZE;
                page1 = radix_get(&reg->data, page_no1);
                page2 = radix_get(&reg->data, page_no2);
                if (page1) {
                        err = radix_del(&reg->data, page_no1, 1);
                        if (err)
                                goto error;
                }
                if (page2) {
                        err = radix_add(&reg->data, page_no1, page2);
                        if (err)
                                goto error;
                        err = radix_del(&reg->data, page_no2, 0);
                        if (err)
                                goto error;
                }
        }

        reg->size -= len;
        return 0;

error:
        error("Error in collapse range!\n");
        return err;
}

/**
 * @brief Zero the given range of a file.
 * @param reg The file to modify.
 * @param offset The start of the range.
 * @param len The length of the range.
 * @param mode The allocate mode of this call.
 * @return 0 on success, -ENOSPC when out of disk(memory) space.
 * @note The range will be set to zero, missing pages will be allocated. If
 * FALLOC_FL_KEEP_SIZE is set in mode, the file size will not be changed.
 */
static int tmpfs_file_zero_range(struct inode *reg, off_t offset, off_t len,
                                 mode_t mode)
{
#if DEBUG
        BUG_ON(reg->type != FS_REG);
#endif

        u64 page_no, page_off;
        u64 cur_off = offset;
        off_t length = len;
        off_t to_zero;
        void *page;

        while (len > 0) {
                page_no = cur_off / PAGE_SIZE;
                page_off = cur_off % PAGE_SIZE;

                to_zero = MIN(len, PAGE_SIZE - page_off);
                cur_off += to_zero;
                len -= to_zero;
                if (!len)
                        to_zero = PAGE_SIZE;
                page = radix_get(&reg->data, page_no);
                if (!page) {
                        page = aligned_alloc(PAGE_SIZE, PAGE_SIZE);
                        if (!page)
                                return -ENOSPC;
#if DEBUG_MEM_USAGE
                        tmpfs_record_mem_usage(page, PAGE_SIZE, DATA_PAGE);
#endif
                        radix_add(&reg->data, page_no, page);
                }

#if DEBUG
                BUG_ON(!page);
#endif

                memset(page + page_off, 0, to_zero);
        }

        if ((!(mode & FALLOC_FL_KEEP_SIZE)) && (offset + length > reg->size))
                reg->size = offset + length;

        return 0;
}

/**
 * @brief Increasing a file's space.
 * @param reg The file to modify.
 * @param offset The start of the range.
 * @param len The length of the range.
 * @return int 0 on success, -EINVAL if the granularity not match or the params
 are illegal, or -1 if radix_del() fails, or -ENOMEM if radix_add() fails.
 * @note The operation inserts spaces at offset with length len, that is: the
 content starting at the offset will be moved to offset + len, and the size of
 the file increased by len. Tmpfs only allows this to happen at page
 granularity.
 */
static int tmpfs_file_insert_range(struct inode *reg, off_t offset, off_t len)
{
#if DEBUG
        BUG_ON(reg->type != FS_REG);
#endif

        u64 page_no1, page_no2;
        void *page;
        int err;
        off_t dist;

        /* To ensure efficient implementation, this mode has the same
         * limitations as FALLOC_FL_COLLAPSE_RANGE regarding the granularity of
         * the operation. (offset and len must be a mutiple of the filesystem
         * logical block size) */
        if (offset % PAGE_SIZE || len % PAGE_SIZE)
                return -EINVAL;
        /* If the offset is equal to or greater than the EOF, an error is
         * returned. For such operations, ftruncate should be used. */
        if (offset >= reg->size)
                return -EINVAL;

        page_no1 = (reg->size + PAGE_SIZE - 1) / PAGE_SIZE;
        dist = len / PAGE_SIZE;
        while (page_no1 >= offset / PAGE_SIZE) {
                page_no2 = page_no1 + dist;
#if DEBUG
                BUG_ON(radix_get(&reg->data, page_no2));
#endif
                page = radix_get(&reg->data, page_no1);
                if (page) {
                        err = radix_del(&reg->data, page_no1, 0);
                        if (err)
                                goto error;
                        err = radix_add(&reg->data, page_no2, page);
                        if (err)
                                goto error;
                }
                page_no1--;
        }

        reg->size += len;
        return 0;

error:
        error("Error in insert range!\n");
        return err;
}

/**
 * @brief Allocate disk space(memory in tmpfs) for the file, from offset to
 * offset + len, and zero any newly allocated memory space. This ensures later
 * operations within the allocated range will not fail because of lack of memory
 * space.
 * @param reg The file to operate with.
 * @param offset The operation starts at offset.
 * @param len The length of the allocated range.
 * @param keep_size If keep_size is set, the file size will not be changed.
 * Otherwise, if offset + len > reg->size, reg->size will be changed to offset +
 * len.
 * @return int 0 on success, -ENOSPC if not enough disk(memory in tmpfs) space.
 */
static int tmpfs_file_allocate(struct inode *reg, off_t offset, off_t len,
                               int keep_size)
{
#if DEBUG
        BUG_ON(reg->type != FS_REG);
#endif

        u64 page_no;
        u64 cur_off = offset;
        void *page;

        while (cur_off < offset + len) {
                page_no = cur_off / PAGE_SIZE;

                page = radix_get(&reg->data, page_no);
                if (!page) {
                        page = aligned_alloc(PAGE_SIZE, PAGE_SIZE);
                        if (!page)
                                return -ENOSPC;
#if DEBUG_MEM_USAGE
                        tmpfs_record_mem_usage(page, PAGE_SIZE, DATA_PAGE);
#endif

                        if (radix_add(&reg->data, page_no, page)) {
#if DEBUG_MEM_USAGE
                                tmpfs_revoke_mem_usage(page, DATA_PAGE);
#endif
                                free(page);
                                return -ENOSPC;
                        }
                        memset(page, 0, PAGE_SIZE);
                }
                cur_off += PAGE_SIZE;
        }

        if (offset + len > reg->size && !keep_size) {
                reg->size = offset + len;
        }
        return 0;
}

/* Directory operations */

/**
 * @brief Allocate a dentry, and initialize it with the parent dentry.
 * @param d_parent The parent dentry of the to-be-allocated dentry.
 * @return The newly created dentry
 */
static struct dentry *tmpfs_alloc_dent(void)
{
        struct dentry *dentry = malloc(sizeof(struct dentry));
        if (!dentry) {
                return CHCORE_ERR_PTR(-ENOMEM);
        }

#if DEBUG_MEM_USAGE
        tmpfs_record_mem_usage(dentry, sizeof(struct dentry), DENTRY);
#endif

        dentry->inode = NULL;
        dentry->name.str = NULL;
        return dentry;
}

/**
 * @brief Free a dentry and its name string.
 * @param inode The inode which does this operation, and should be the
 * to-be-freed dentry's parent directory.
 * @param dentry The dentry that is to be freed.
 */
static void tmpfs_free_dent(struct dentry *dentry)
{
        free_string(&dentry->name);

#if DEBUG_MEM_USAGE
        tmpfs_revoke_mem_usage(dentry, DENTRY);
#endif
        free(dentry);
}

/**
 * @brief Add a just allocated dentry into a directory, and adjust the dir's
 * size.
 * @param dir The directory to add the new dentry.
 * @param new_dent The newly created dentry that will be added to the dir.
 * @param name The name of the dentry.
 * @param len The length of the name.
 * @return int 0 for success, -ENOMEM if init_string() fails.
 * @note This function does not create the new dentry, it presuppose a created
 * dentry.
 */
static int tmpfs_dir_add_dent(struct inode *dir, struct dentry *new_dent,
                              char *name, size_t len)
{
#if DEBUG
        BUG_ON(dir->type != FS_DIR);
#endif

        int err;

        err = init_string(&new_dent->name, name, len); /* copy the str */
        if (err) {
                return err;
        }

        htable_add(&dir->dentries, (u32)(new_dent->name.hash), &new_dent->node);
        dir->size += DENT_SIZE;

        return 0;
}

/**
 * @brief Remove a dentry from the dir's htable, adjust the size.
 * @param dir the directory to remove the dentry from.
 * @param dentry the dentry to be removed.
 * @note This operation only operate on the directory, *dentry is not freed
 */
static void tmpfs_dir_remove_dent(struct inode *dir, struct dentry *dentry)
{
#if DEBUG
        BUG_ON(dir->type != FS_DIR);
#endif

        htable_del(&dentry->node);
        dir->size -= DENT_SIZE;
        /* not freeing the dentry now */
}

/**
 * @brief Check if the directory is an empty and legal one.
 * @param dir The directory to be checked.
 * @return bool true for empty and legal, false otherwise.
 * @note By saying empty and legal, we mean the directory should *have and only
 * have* the dot dentry and the dotdot dentry in its dentries table.
 */
static bool tmpfs_dir_empty(struct inode *dir)
{
#if DEBUG
        BUG_ON(dir->type != FS_DIR);
#endif

        struct dentry *iter;
        int i = 0;
        bool found_dot, found_dotdot;

        found_dot = false;
        found_dotdot = false;

        for_each_in_htable (iter, i, node, &dir->dentries) {
                if (strcmp(iter->name.str, ".") == 0) {
                        found_dot = true;
                } else if (strcmp(iter->name.str, "..") == 0) {
                        found_dotdot = true;
                } else {
                        /* dentry other than "." or "..", dir not empty */
                        return false;
                }
        }

        return found_dot && found_dotdot;
}

/**
 * @brief Link the dentry with the inode, and do nothing else.
 * @param dir the parent directory which the dentry belongs to.
 * @param dentry The dentry, which has already been added to the directory, to
 * hold the inode.
 * @param inode The inode to be linked with the dentry.
 */
static void tmpfs_dir_link(struct inode *dir, struct dentry *dentry,
                           struct inode *inode)
{
#if DEBUG
        BUG_ON(dir->type != FS_DIR);
#endif

        dentry->inode = inode;
        inode->base_ops->inc_nlinks(inode);
}

/**
 * @brief Unlink the dentry with its inode, and also remove the dentry from the
 * directory and free the dentry.
 * @param dir The parent directory which has the dentry in it.
 * @param dentry The dentry to be unlinked.
 */
static void tmpfs_dir_unlink(struct inode *dir, struct dentry *dentry)
{
#if DEBUG
        BUG_ON(dir->type != FS_DIR);
        BUG_ON(dentry->inode == NULL);
#endif

        struct inode *inode = dentry->inode;

        dir->d_ops->remove_dentry(dir, dentry);
        dir->d_ops->free_dentry(dentry);
        inode->base_ops->dec_nlinks(inode);
}

/**
 * @brief Make a new directory under a parent dir.
 * @param dir The parent dir to hold the new directory.
 * @param dentry The dentry of the new directory.
 * @param mode The access mode of the dir, ignored.
 * @return int 0 on success, -ENOMEM on failure.
 * @return inode Upon success, a new directory inode is created and can be
 * accessed by dentry->inode.
 */
static int tmpfs_dir_mkdir(struct inode *dir, struct dentry *dentry,
                           mode_t mode)
{
#if DEBUG
        BUG_ON(dir->type != FS_DIR);
#endif

        int err = dir->d_ops->mknod(dir, dentry, mode, FS_DIR);
        if (err) {
                return err;
        }

        struct inode *new_dir = dentry->inode;

        struct dentry *dot = new_dir->d_ops->alloc_dentry();
        if (CHCORE_IS_ERR(dot)) {
                err = CHCORE_PTR_ERR(dot);
                goto free_node;
        }

        err = new_dir->d_ops->add_dentry(new_dir, dot, ".", 1);
        if (err) {
                goto free_dot;
        }

        new_dir->d_ops->link(new_dir, dot, new_dir);

        struct dentry *dotdot = new_dir->d_ops->alloc_dentry();
        if (CHCORE_IS_ERR(dotdot)) {
                err = CHCORE_PTR_ERR(dotdot);
                goto remove_dot;
        }

        err = new_dir->d_ops->add_dentry(new_dir, dotdot, "..", 2);
        if (err) {
                goto free_dotdot;
        }

        new_dir->d_ops->link(new_dir, dotdot, dir);

        return 0;

free_dotdot:
        new_dir->d_ops->free_dentry(dotdot);
remove_dot:
        new_dir->d_ops->remove_dentry(new_dir, dot);
free_dot:
        new_dir->d_ops->free_dentry(dot);
free_node:
        dentry->inode->base_ops->free(dentry->inode);
        /* the caller-allocated dentry is not freed */
        dentry->inode = NULL;

        return err;
}

/**
 * @brief Remove an empty directory and its dentry under the parent directory.
 * @param dir The parent directory which holds the to-be-removed directory.
 * @param dentry The dentry of the directory to remove.
 * @return int 0 on success, -ENOTEMPTY if the to-be-removed directory is not
 * empty.
 */
static int tmpfs_dir_rmdir(struct inode *dir, struct dentry *dentry)
{
#if DEBUG
        BUG_ON(dir->type != FS_DIR);
        BUG_ON(dentry->inode->type != FS_DIR);
#endif

        struct inode *to_remove = dentry->inode;

        if (!to_remove->d_ops->is_empty(to_remove)) {
                return -ENOTEMPTY;
        }

        struct dentry *dot = to_remove->d_ops->dirlookup(to_remove, ".", 1);
        struct dentry *dotdot = to_remove->d_ops->dirlookup(to_remove, "..", 2);

#if DEBUG
        BUG_ON(dot == NULL || dotdot == NULL);
#endif

        to_remove->d_ops->unlink(to_remove, dot);
        to_remove->d_ops->unlink(to_remove, dotdot);

        dir->d_ops->unlink(dir, dentry);

        return 0;
}

/*
 * All illegal cases have been handled outside
 * Call to this function is guaranteed to be legal and will succeed
 */
/**
 * @brief Rename an inode by reclaiming its dentry.
 * @param old_dir The parent directory where the inode was under.
 * @param old_dentry The dentry for the inode under old_dir.
 * @param new_dir The parent directory where the inode is moving to.
 * @param new_dentry The dentry that the inode is moving to. Should be added to
 * the new_dir already.
 * @note The rename() system call is very different with this simple function
 * because of all the corner cases and checks. This operation is simple because
 * it presupposes all the checks have been done outside before the call to this
 * function so it can only handle the legal case and is guaranteed to succeed.
 */
static void tmpfs_dir_rename(struct inode *old_dir, struct dentry *old_dentry,
                             struct inode *new_dir, struct dentry *new_dentry)
{
#if DEBUG
        BUG_ON(new_dir->type != FS_DIR);
        BUG_ON(old_dir->type != FS_DIR);
#endif

        /* we just link the new_dentry to the inode and unlink the old_dentry */
        struct inode *inode = old_dentry->inode;

        new_dir->d_ops->link(new_dir, new_dentry, inode); /* link first */
        old_dir->d_ops->unlink(old_dir, old_dentry);

        if (inode->type == FS_DIR) {
                struct dentry *dotdot = inode->d_ops->dirlookup(inode, "..", 2);

#if DEBUG
                struct dentry *dot = inode->d_ops->dirlookup(inode, ".", 1);
                BUG_ON(!dot);
                BUG_ON(!dotdot);
                BUG_ON(dot->inode != inode);
                BUG_ON(dotdot->inode != old_dir);
#endif

                dotdot->inode = new_dir;

                /* dotdot is changed*/
                old_dir->base_ops->dec_nlinks(old_dir);
                new_dir->base_ops->inc_nlinks(new_dir);
        }
}

/**
 * @brief Make a new inode under the parent directory.
 * @param dir The parent directory to hold the newly created inode.
 * @param dentry The dentry for the new inode. It should be already allocated
 * and added into the dir.
 * @param mode The access mode of this new inode. Ignored.
 * @param type The type of the inode. FS_REG, FS_DIR, FS_SYM can be used.
 * @return int 0 on success, -ENOMEM if the type is wrong or no enough memory.
 * @return inode On success, the newly created inode is returned and can be
 * accessed in dentry->inode.
 */
static int tmpfs_dir_mknod(struct inode *dir, struct dentry *dentry,
                           mode_t mode, int type)
{
#if DEBUG
        BUG_ON(dir->type != FS_DIR);
#endif

        struct inode *inode = tmpfs_inode_init(type, mode);
        if (inode == NULL) {
                return -ENOMEM;
        }

        /* linking the inode and the dentry */
        dir->d_ops->link(dir, dentry, inode);

        return 0;
}

/**
 * @brief Lookup a given dentry name under the directory.
 * @param dir The directory to lookup.
 * @param name The name of the dentry to find.
 * @param len The length of the dentry name.
 * @return dentry NULL if not found, a pointer to the dentry if found.
 */
static struct dentry *tmpfs_dirlookup(struct inode *dir, const char *name,
                                      size_t len)
{
#if DEBUG
        BUG_ON(dir->type != FS_DIR);
#endif
        u64 hash;
        struct hlist_head *head;
        struct dentry *iter;

        hash = hash_chars(name, len);

        head = htable_get_bucket(&dir->dentries, (u32)hash);
        for_each_in_hlist (iter, node, head) {
                if (iter->name.len == len
                    && strncmp(iter->name.str, name, len) == 0) {
                        return iter;
                }
        }

        return NULL;
}

#define DIRENT_NAME_MAX 256

/**
 * @brief Fill a dirent with some metadata.
 * @param dirpp The pointer of pointer of the dirent struct.
 * @param end The end of the array of dirent.
 * @param name The name of the dirent.
 * @param off The offset of the dirent in the directory.
 * @param type The type of the dirent.
 * @param ino The size of the inode.
 * @return int The length of the written data.
 */
static int tmpfs_dir_fill_dirent(void **dirpp, void *end, char *name, off_t off,
                                 unsigned char type, ino_t ino)
{
        struct dirent *dirp = *(struct dirent **)dirpp;
        void *p = dirp;
        unsigned short len = sizeof(struct dirent);
        p += len;
        if (p > end)
                return -EAGAIN;
        dirp->d_ino = ino;
        dirp->d_off = off;
        dirp->d_reclen = len;
        dirp->d_type = type;
        strlcpy(dirp->d_name, name, DIRENT_NAME_MAX);
        *dirpp = p;
        return len;
}

/**
 * @brief Scan a directory's dentries and write them into a buffer.
 * @param dir The directory to scan.
 * @param start The index in directory's dentry table to scan with.
 * @param buf The caller provided buffer of the dirent array.
 * @param end The end of the dirent array.
 * @return int The number of dentries scanned.
 * @return read_bytes The number of bytes written to the buffer.
 */
static int tmpfs_dir_scan(struct inode *dir, unsigned int start, void *buf,
                          void *end, int *read_bytes)
{
#if DEBUG
        BUG_ON(dir->type != FS_DIR);
#endif

        int cnt = 0, b, ret;
        ino_t ino;
        void *p = buf;
        unsigned char type;
        struct dentry *iter;

        for_each_in_htable (iter, b, node, &dir->dentries) {
                if (cnt >= start) {
                        type = iter->inode->type;
                        ino = iter->inode->size;

                        ret = tmpfs_dir_fill_dirent(
                                &p, end, iter->name.str, cnt, type, ino);

                        if (ret <= 0) {
                                if (read_bytes) {
                                        *read_bytes = (int)(p - buf);
                                }
                                return (int)(cnt - start);
                        }
                }
                cnt++;
        }

        if (read_bytes) {
                *read_bytes = (int)(p - buf);
        }
        return (int)(cnt - start);
}

/* Symlink operations*/

/**
 * @brief Read the content of a symlink
 * @param symlink The symlink.
 * @param buff The buffer to read the symlink into.
 * @param len The length of the buffer
 * @return ssize_t The actual bytes read into the buffer.
 */
static ssize_t tmpfs_symlink_read(struct inode *symlink, char *buff, size_t len)
{
#if DEBUG
        BUG_ON(symlink->type != FS_SYM);
#endif

        len = len < symlink->size ? len : symlink->size;
        memcpy(buff, symlink->symlink, len);
        return (ssize_t)len;
}

/**
 * @brief Write a symlink.
 * @param symlink The symlink.
 * @param buff The buffer of the content to be written.
 * @param len The length of the symlink to be written.
 * @return ssize_t The length of the link successfully written, or -ENAMETOOLONG
 * if the link is too long, or -ENOMEM if there is no enough memory for the link
 */
static ssize_t tmpfs_symlink_write(struct inode *symlink, const char *buff,
                                   size_t len)
{
#if DEBUG
        BUG_ON(symlink->type != FS_SYM);
#endif

        if (len > MAX_SYM_LEN) {
                return -ENAMETOOLONG;
        }

        if (symlink->symlink) {
#if DEBUG_MEM_USAGE
                tmpfs_revoke_mem_usage(symlink->symlink, SYMLINK);
#endif
                free(symlink->symlink);
        }

        symlink->symlink = malloc(len + 1);
        if (!symlink->symlink) {
                return -ENOMEM;
        }
#if DEBUG_MEM_USAGE
        tmpfs_record_mem_usage(symlink->symlink, len + 1, SYMLINK);
#endif

        memcpy(symlink->symlink, buff, len);
        symlink->symlink[len] = '\0';

        symlink->size = (off_t)len;

        return (ssize_t)len;
}

struct base_inode_ops base_inode_ops = {
        .open = tmpfs_inode_open,
        .close = tmpfs_inode_close,
        .inc_nlinks = tmpfs_inode_inc_nlinks,
        .dec_nlinks = tmpfs_inode_dec_nlinks,
        .free = tmpfs_inode_free,
        .stat = tmpfs_inode_stat,
};

struct regfile_ops regfile_ops = {
        .read = tmpfs_file_read,
        .write = tmpfs_file_write,
        .truncate = tmpfs_file_truncate,
        .punch_hole = tmpfs_file_punch_hole,
        .collapse_range = tmpfs_file_collapse_range,
        .zero_range = tmpfs_file_zero_range,
        .insert_range = tmpfs_file_insert_range,
        .allocate = tmpfs_file_allocate,
};

struct dir_ops dir_ops = {
        .alloc_dentry = tmpfs_alloc_dent,
        .free_dentry = tmpfs_free_dent,
        .add_dentry = tmpfs_dir_add_dent,
        .remove_dentry = tmpfs_dir_remove_dent,
        .is_empty = tmpfs_dir_empty,
        .dirlookup = tmpfs_dirlookup,
        .mknod = tmpfs_dir_mknod,
        .link = tmpfs_dir_link,
        .unlink = tmpfs_dir_unlink,
        .mkdir = tmpfs_dir_mkdir,
        .rmdir = tmpfs_dir_rmdir,
        .rename = tmpfs_dir_rename,
        .scan = tmpfs_dir_scan,
};

struct symlink_ops symlink_ops = {
        .read_link = tmpfs_symlink_read,
        .write_link = tmpfs_symlink_write,
};
