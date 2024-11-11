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

#include <stdio.h>
#include <sys/mount.h>

int main(int argc, char *argv[])
{
        if (argc < 3) {
                printf("example: mount.bin sda1 /home\n");
                return 1;
        }

        const char *dev = argv[1];
        const char *mountpoint = argv[2];

        printf("mount \"%s\" to \"%s\"\n", dev, mountpoint);
        int ret = mount(dev, mountpoint, NULL, 0, NULL);
        if (ret == 0) {
                printf("succeeded\n");
        } else {
                printf("failed\n");
        }
        return 0;
}
