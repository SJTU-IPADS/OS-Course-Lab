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

#include <dev_messenger.h>
#include <uvc_messenger.h>
#include <chcore-internal/usb_devmgr_defs.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <assert.h>

int main()
{
        int devnum;
        int shmid;
        u8 *shmaddr;
        int shmkey;
        int ret;
        struct dev_desc *dev_desc;

        struct dev_info *dev_info;
        devnum = dm_list_dev(&dev_info);

        for (int i = 0; i < devnum; i++) {
                printf("[list device]: devname:%s, dev_class:%d, vendor:%s\n",
                       dev_info[i].devname,
                       dev_info[i].dev_class,
                       dev_info[i].vendor);
        }

        if ((dev_desc = dm_open("UVC-0", 0)) == NULL) {
                printf("Error occurs when openning device.\n");
                return 0;
        }
        printf("successfully open UVC0\n");

        // prepare shared memory
        shmkey = 0x123;
        shmid = shmget(shmkey, JPEG_IMG_BUF_SIZE, IPC_CREAT);
        shmaddr = (u8 *)shmat(shmid, 0, 0);
        ret = dm_ctrl(dev_desc, UVC_CAPTURE_IMG, &shmkey, 0);

        for (int j = 0; j < ret; j++) {
                printf("%02X ", shmaddr[j]);
                if ((j + 1) % 50 == 0) {
                        printf("\n");
                }
        }

        dm_close(dev_desc);
        return 0;
}
