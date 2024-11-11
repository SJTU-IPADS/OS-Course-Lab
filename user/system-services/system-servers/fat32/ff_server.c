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

#include <bits/errno.h>
#include <chcore/type.h>
#include <chcore/ipc.h>
#include <chcore/bug.h>
#include <chcore/syscall.h>
#include <pthread.h>
#include <malloc.h>
#include <assert.h>
#include <chcore-internal/fs_defs.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <string.h>

#include <fs_wrapper_defs.h>
#include <chcore-internal/fs_debug.h>
#include <fs_vnode.h>
#include "ff.h"
#include "diskio.h"
#include "fat32_defs.h"

char sd[4];
FATFS *fs;
pthread_mutex_t fat_meta_lock;
bool mounted;
bool using_page_cache;

#ifdef TEST_COUNT_PAGE_CACHE
struct test_count count;
#endif

struct server_entry *server_entrys[MAX_SERVER_ENTRY_NUM];
struct rb_root *fs_vnode_list;

static inline void init_fat32_server(void)
{
        /* Initialize for sd IPC */
        sd_ipc_init();

        /* Initilaize fs_wrapper */
        init_fs_wrapper();
        using_page_cache = true;

        /* Initialize private datas */
        strcpy(sd, "sd0");
        mounted = false;
        pthread_mutex_init(&fat_meta_lock, NULL);

        /* Just initialize FATFS struct, and fill it when receive FS_REQ_MOUNT
         */
        fs = (FATFS *)malloc(sizeof(FATFS));
        BUG_ON(fs == NULL);
        memset((unsigned char *)fs, 0, sizeof(FATFS));
}

int main(void)
{
        int ret;

        init_fat32_server();

        ret = ipc_register_server_with_destructor(
                fs_server_dispatch,
                DEFAULT_CLIENT_REGISTER_HANDLER,
                fs_server_destructor);
        printf("[Fat fs] register server value = %d\n", ret);

        while (1) {
                sched_yield();
        }

        return 0;
}
