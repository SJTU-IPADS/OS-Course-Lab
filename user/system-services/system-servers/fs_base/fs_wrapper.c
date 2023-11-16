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

#include "chcore/ipc.h"
#include <errno.h>
#include <pthread.h>
#include <chcore/bug.h>
#include <chcore/type.h>
#include <chcore/memory.h>
#include <chcore-internal/fs_defs.h>
#include <chcore-internal/fs_debug.h>
#include <sys/mman.h>
#include "fs_wrapper_defs.h"
#include "fs_page_cache.h"
#include "fs_vnode.h"
#include "fs_page_fault.h"

/* fs server private data */
struct list_head server_entry_mapping;
pthread_spinlock_t server_entry_mapping_lock;
pthread_rwlock_t fs_wrapper_meta_rwlock;

/* +++++++++++++++++++++++++++ Initializing +++++++++++++++++++++++++++++++ */

int real_file_reader(char *buffer, pidx_t file_page_idx, void *private)
{
        struct fs_vnode *vnode;
        size_t size;
        off_t offset;

        vnode = (struct fs_vnode *)private;

        size = CACHED_PAGE_SIZE;
        offset = file_page_idx * CACHED_PAGE_SIZE;

        /* buffer size should always be PAGE_SIZE. */
        memset(buffer, 0, size);

        if (offset + size > vnode->size)
                size = vnode->size - offset;
#ifdef TEST_COUNT_PAGE_CACHE
        count.disk_o = count.disk_o + size;
#endif
        return server_ops.read(vnode->private, offset, size, buffer);
}

int real_file_writer(char *buffer, pidx_t file_page_idx, int page_block_idx,
                     void *private)
{
        struct fs_vnode *vnode;
        off_t offset;
        size_t size;

        vnode = (struct fs_vnode *)private;
        offset = file_page_idx * CACHED_PAGE_SIZE;
        if (page_block_idx == -1) {
                size = CACHED_PAGE_SIZE;
        } else {
                size = CACHED_BLOCK_SIZE;
                offset += page_block_idx * CACHED_BLOCK_SIZE;
        }

        if (offset + size > vnode->size)
                size = vnode->size - offset;
#ifdef TEST_COUNT_PAGE_CACHE
        count.disk_i = count.disk_i + size;
#endif
        return server_ops.write(vnode->private, offset, size, buffer);
}

void init_fs_wrapper(void)
{
        struct user_defined_funcs uf;

        /* fs wrapper */
        init_list_head(&server_entry_mapping);
        pthread_spin_init(&server_entry_mapping_lock, 0);
        fs_vnode_init();
        pthread_rwlock_init(&fs_wrapper_meta_rwlock, NULL);

        uf.file_read = real_file_reader;
        uf.file_write = real_file_writer;
        uf.handler_pce_turns_empty = dec_ref_fs_vnode;
        uf.handler_pce_turns_nonempty = inc_ref_fs_vnode;

        fs_page_cache_init(WRITE_THROUGH, &uf);

        /* Module: fmap fault */
        fs_page_fault_init();

#ifdef TEST_COUNT_PAGE_CACHE
        count.hit = 0;
        count.miss = 0;
        count.disk_i = 0;
        count.disk_o = 0;
#endif
}

/* +++++++++++++++++++++++++++ FID Mapping ++++++++++++++++++++++++++++++++ */

/* Get (client_badge, fd) -> fid(server_entry) mapping */
int fs_wrapper_get_server_entry(badge_t client_badge, int fd)
{
        struct server_entry_node *n;

        /* Stable fd number, need no translating */
        if (fd == AT_FDROOT)
                return AT_FDROOT;

        /* Validate fd */
        if (fd < 0 || fd >= MAX_SERVER_ENTRY_PER_CLIENT) {
                return -1;
        }

        /* Lab 5 TODO Begin */

        UNUSED(n);

        /* Lab 5 TODO End */
        return -1;
}

/* Set (client_badge, fd) -> fid(server_entry) mapping */
int fs_wrapper_set_server_entry(badge_t client_badge, int fd, int fid)
{
        struct server_entry_node *private_iter;

        /* Validate fd */
        BUG_ON(fd < 0 || fd >= MAX_SERVER_ENTRY_PER_CLIENT);

        /* Lab 5 TODO Begin */

        /* 
         * Check if client_badge already involved, 
         * create new server_entry_node if not.
         */

        UNUSED(private_iter);
        
        /* Lab 5 TODO End */
        return 0;
}

void fs_wrapper_clear_server_entry(badge_t client_badge, int fid)
{
        struct server_entry_node *private_iter;

        /* Check if client_badge already involved */
        pthread_spin_lock(&server_entry_mapping_lock);
        for_each_in_list (private_iter,
                          struct server_entry_node,
                          node,
                          &server_entry_mapping) {
                if (private_iter->client_badge == client_badge) {
                        for (int i = 0; i < MAX_SERVER_ENTRY_NUM; i++) {
                                if (private_iter->fd_to_fid[i] == fid) {
                                        private_iter->fd_to_fid[i] = -1;
                                }
                        }
                        pthread_spin_unlock(&server_entry_mapping_lock);
                        return;
                }
        }
        pthread_spin_unlock(&server_entry_mapping_lock);
}

#define translate_or_noent(badge, fd)                       \
        do {                                                \
                int r;                                      \
                r = fs_wrapper_get_server_entry(badge, fd); \
                if (r < 0)                                  \
                        ret = -ENOENT;                      \
                else                                        \
                        (fd) = r;                           \
        } while (0)

/* Translate xxxfd field to fid correspondingly */
int translate_fd_to_fid(badge_t client_badge, struct fs_request *fr)
{
        int ret = 0;
        /* Except FS_REQ_OPEN and FS_REQ_MOUNT, fd should be translated */
        if (fr->req == FS_REQ_OPEN || fr->req == FS_REQ_MOUNT)
                return ret;

        switch (fr->req) {
        case FS_REQ_FSTATAT:
        case FS_REQ_FSTAT:
        case FS_REQ_FSTATFS:
        case FS_REQ_STATFS:
                fr->stat.dirfd = fs_wrapper_get_server_entry(client_badge,
                                                             fr->stat.dirfd);
                fr->stat.fd =
                        fs_wrapper_get_server_entry(client_badge, fr->stat.fd);

                if ((fr->stat.fd < 0 && fr->stat.fd != AT_FDROOT)
                    || (fr->stat.dirfd < 0 && fr->stat.dirfd != AT_FDROOT)) {
                        ret = -ENOENT;
                }

                break;
        case FS_REQ_READ:
                translate_or_noent(client_badge, fr->read.fd);
                break;
        case FS_REQ_WRITE:
                translate_or_noent(client_badge, fr->write.fd);
                break;
        case FS_REQ_PREAD:
                translate_or_noent(client_badge, fr->pread.fd);
                break;
        case FS_REQ_PWRITE:
                translate_or_noent(client_badge, fr->pwrite.fd);
                break;
        case FS_REQ_LSEEK:
                translate_or_noent(client_badge, fr->lseek.fd);
                break;
        case FS_REQ_CLOSE:
                translate_or_noent(client_badge, fr->close.fd);
                break;
        case FS_REQ_FTRUNCATE:
                translate_or_noent(client_badge, fr->ftruncate.fd);
                break;
        case FS_REQ_FALLOCATE:
                translate_or_noent(client_badge, fr->fallocate.fd);
                break;
        case FS_REQ_FCNTL:
                translate_or_noent(client_badge, fr->fcntl.fd);
                break;
        case FS_REQ_FSYNC:
                translate_or_noent(client_badge, fr->fsync.fd);
                break;
        case FS_REQ_FDATASYNC:
                translate_or_noent(client_badge, fr->fdatasync.fd);
                break;
        case FS_REQ_FMAP:
                translate_or_noent(client_badge, fr->mmap.fd);
                break;
        case FS_REQ_GETDENTS64:
                translate_or_noent(client_badge, fr->getdents64.fd);
                break;
        default:
                break;
        }

        return ret;
}

DEFINE_SERVER_HANDLER(fs_server_dispatch)
{
        struct fs_request *fr;
        long ret;
        bool ret_with_cap = false;

        if (ipc_msg == NULL) {
                ipc_return(ipc_msg, -EINVAL);
        }

        fr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        /* We only support concurrent READ and WRITE */
        if (fr->req != FS_REQ_READ && fr->req != FS_REQ_WRITE) {
                pthread_rwlock_wrlock(&fs_wrapper_meta_rwlock);
        } else {
                pthread_rwlock_rdlock(&fs_wrapper_meta_rwlock);
        }

        /*
         * Some FS Servers need to complete the initialization process when
         * mounting eg. Connect with corresponding block device, Save partition
         * offset, etc So, when the mounted flag is off, requests will be
         * rejected except FS_REQ_MOUNT
         */
        if (!mounted && (fr->req != FS_REQ_MOUNT)) {
                printf("[fs server] Not fully initialized, send FS_REQ_MOUNT first\n");
                ret = -EINVAL;
                goto out;
        }

        /*
         * Now fr->fd stores the `Client Side FD Index',
         * We need to translate fr->fd to fid here, except FS_REQ_OPEN
         * FS_REQ_OPEN's fr->fd stores the newly generated `Client Side FD
         * Index' and we should build mapping between fr->fd to fid when handle
         * open request
         */
        ret = translate_fd_to_fid(client_badge, fr);
        if (ret < 0) {
                goto out;
        }

        /*
         * FS Server Requests Handlers
         */
        switch (fr->req) {
        case FS_REQ_MOUNT:
                ret = fs_wrapper_mount(ipc_msg, fr);
                break;
        case FS_REQ_UMOUNT:
                ret = fs_wrapper_umount(ipc_msg, fr);
                break;
        case FS_REQ_OPEN:
                ret = fs_wrapper_open(client_badge, ipc_msg, fr);
                break;
        case FS_REQ_READ:
                ret = fs_wrapper_read(ipc_msg, fr);
                break;
        case FS_REQ_WRITE:
                ret = fs_wrapper_write(ipc_msg, fr);
                break;
        case FS_REQ_PREAD:
                ret = fs_wrapper_pread(ipc_msg, fr);
                break;
        case FS_REQ_PWRITE:
                ret = fs_wrapper_pwrite(ipc_msg, fr);
                break;
        case FS_REQ_LSEEK:
                ret = fs_wrapper_lseek(ipc_msg, fr);
                break;
        case FS_REQ_CLOSE:
                ret = fs_wrapper_close(client_badge, ipc_msg, fr);
                break;
        case FS_REQ_CREAT:
                ret = fs_wrapper_creat(ipc_msg, fr);
                break;
        case FS_REQ_UNLINK:
                ret = fs_wrapper_unlink(ipc_msg, fr);
                break;
        case FS_REQ_RMDIR:
                ret = fs_wrapper_rmdir(ipc_msg, fr);
                break;
        case FS_REQ_MKDIR:
                ret = fs_wrapper_mkdir(ipc_msg, fr);
                break;
        case FS_REQ_RENAME:
                ret = fs_wrapper_rename(ipc_msg, fr);
                break;
        case FS_REQ_GETDENTS64:
                ret = fs_wrapper_getdents64(ipc_msg, fr);
                break;
        case FS_REQ_FTRUNCATE:
                ret = fs_wrapper_ftruncate(ipc_msg, fr);
                break;
        case FS_REQ_FSTATAT:
                ret = fs_wrapper_fstatat(ipc_msg, fr);
                break;
        case FS_REQ_FSTAT:
                ret = fs_wrapper_fstat(ipc_msg, fr);
                break;
        case FS_REQ_STATFS:
                ret = fs_wrapper_statfs(ipc_msg, fr);
                break;
        case FS_REQ_FSTATFS:
                ret = fs_wrapper_fstatfs(ipc_msg, fr);
                break;
        case FS_REQ_FACCESSAT:
                ret = fs_wrapper_faccessat(ipc_msg, fr);
                break;
        case FS_REQ_SYMLINKAT:
                ret = fs_wrapper_symlinkat(ipc_msg, fr);
                break;
        case FS_REQ_READLINKAT:
                ret = fs_wrapper_readlinkat(ipc_msg, fr);
                break;
        case FS_REQ_FALLOCATE:
                ret = fs_wrapper_fallocate(ipc_msg, fr);
                break;
        case FS_REQ_FCNTL:
                ret = fs_wrapper_fcntl(client_badge, ipc_msg, fr);
                break;
        case FS_REQ_FMAP:
                ret = fs_wrapper_fmap(client_badge, ipc_msg, fr, &ret_with_cap);
                break;
        case FS_REQ_SYNC:
                ret = fs_wrapper_sync();
                break;
        case FS_REQ_FSYNC:
        case FS_REQ_FDATASYNC:
                ret = fs_wrapper_fsync(ipc_msg, fr);
                break;
        case FS_REQ_TEST_PERF:
                ret = fs_wrapper_count(ipc_msg, fr);
                break;
        default:
                printf("[Error] Strange FS Server request number %d\n",
                       fr->req);
                ret = -EINVAL;
                break;
        }

out:
        pthread_rwlock_unlock(&fs_wrapper_meta_rwlock);
        if (ret_with_cap)
                ipc_return_with_cap(ipc_msg, ret);
        else
                ipc_return(ipc_msg, ret);
}
