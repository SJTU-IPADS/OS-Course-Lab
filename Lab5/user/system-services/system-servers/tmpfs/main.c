/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#include "defs.h"
#include "fcntl.h"
#include "namei.h"
#include "cpio.h"
#include <chcore/ipc.h>
#include <chcore/syscall.h>

#include "../fs_base/fs_wrapper_defs.h"
#include "../fs_base/fs_vnode.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

struct id_manager fidman;
struct fid_record fid_records[MAX_NR_FID_RECORDS];

bool mounted;
bool using_page_cache;

#ifdef TEST_COUNT_PAGE_CACHE
struct test_count count;
#endif

struct server_entry *server_entrys[MAX_SERVER_ENTRY_NUM];
struct rb_root *fs_vnode_list;

int tmpfs_load_image(const char *start)
{
        struct cpio_file *f;
        struct inode *inode;
        struct dentry *dentry;
        struct open_flags open_flags;
        const char *path;
        size_t len;
        int err = 0;
        ssize_t write_count;
        struct nameidata nd;
        mode_t mode;

        BUG_ON(start == NULL);

        cpio_init_g_files();
        cpio_extract(start, "/");

        for (f = g_files.head.next; f; f = f->next) {
                path = f->name;
                mode = f->header.c_mode;

                /* skipping '/'s. maybe this is unnecessary */
                while (*path == '/') {
                        path++;
                }

                if (S_ISDIR(mode)) {
                        err = tmpfs_mkdir(path, mode);
                        /*
                         * cpio file has the "." and ".." dirents in it,
                         * which tmpfs have already created.
                         * so we tolerate the EEXIST error here.
                         */
                        if (err && err != -EEXIST) {
                                goto error;
                        }
                        continue;
                }

                if (!S_ISREG(mode)) {
                        warn("path %s is not a directory nor a regular file, not supported now\n",
                             path);
                        continue;
                }

                /* path points a regular file now */

                open_flags.o_flags = O_CREAT | O_WRONLY;
                open_flags.mode = mode;

                err = path_openat(&nd, path, &open_flags, 0, &dentry);
                if (err) {
                        goto error;
                }

                inode = dentry->inode;

                len = f->header.c_filesize;
                write_count = inode->f_ops->write(inode, f->data, len, 0);
                if (write_count != len) {
                        err = -ENOSPC;
                        goto error;
                }
        }

#ifdef CHCORE_OPENTRUSTEE
        err = tmpfs_mkdir("/tafs", 0);
        BUG_ON(err);
#endif /* CHCORE_OPENTRUSTEE */

error:
        cpio_free_g_files();
        return err;
}

/* init tmpfs root, creating . and .. inside */
void init_root(void)
{
        int err;
        tmpfs_root_dent = malloc(sizeof(struct dentry));
        BUG_ON(!tmpfs_root_dent);

        tmpfs_root = tmpfs_inode_init(FS_DIR, S_IRUSR | S_IWUSR | S_IXUSR);
        assert(tmpfs_root);
        tmpfs_root_dent->inode = tmpfs_root;

        struct dentry *d_root_dot = tmpfs_root->d_ops->alloc_dentry();
        assert(!CHCORE_IS_ERR(d_root_dot));
        err = tmpfs_root->d_ops->add_dentry(tmpfs_root, d_root_dot, ".", 1);
        assert(!err);

        struct dentry *d_root_dotdot = tmpfs_root->d_ops->alloc_dentry();
        assert(!CHCORE_IS_ERR(d_root_dotdot));
        err = tmpfs_root->d_ops->add_dentry(tmpfs_root, d_root_dotdot, "..", 2);
        assert(!err);

        tmpfs_root->d_ops->link(tmpfs_root, d_root_dot, tmpfs_root);
        tmpfs_root->d_ops->link(tmpfs_root, d_root_dotdot, tmpfs_root);
}

static int init_tmpfs(void)
{
        init_root();

        init_id_manager(&fidman, MAX_NR_FID_RECORDS, DEFAULT_INIT_ID);
        assert(alloc_id(&fidman) == 0);

        init_fs_wrapper();

        /* tmpfs should never use page cache. */
        using_page_cache = false;
        BUG_ON(using_page_cache);

        mounted = true;

        return 0;
}

int main(int argc, char *argv[], char *envp[])
{
        init_tmpfs();

        int ret = tmpfs_load_image((char *)&__binary_ramdisk_cpio_start);
        if (ret) {
                BUG_ON(1);
        }

        info("register server value = %u\n",
             ipc_register_server_with_destructor(fs_server_dispatch,
                                                 DEFAULT_CLIENT_REGISTER_HANDLER,
                                                 fs_server_destructor));

        usys_exit(0);
        return 0;
}
