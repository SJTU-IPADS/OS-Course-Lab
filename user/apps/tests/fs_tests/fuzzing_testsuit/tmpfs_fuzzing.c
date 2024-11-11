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

#define _GNU_SOURCE

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <fcntl.h>

static void test0(void)
{ // creat, fstat behind "rename, open" will trigger this bug
        char buff[8192];
        char v9[] = "./eventfd.bin";
        char v17[] = "./pipe.bin";
        int fd;

        rename(v9, v17);
        fd = open(v17, 2);
        fstat(fd, (struct stat*)buff);
}

static void test1(void)
{ /// home/mt/fuzz/chcore-fuzzer/seed/ramdisk1/output3/output2NoLink/crashes/id:000004,sig:06,src:000009,op:fs-havoc-generate,pos:0
        char v8[] = "./fat32.srv";
        char v28[] = "wlan/wpa_supplicant.conf";
        char v31[] = "wlan/l63MhKcJ";

        rename(v8, v28);
        open(v28, 2, 0);
        creat(v31, 20736);
}

static void test2(void)
{ // run 3 times
        char v2[] = ".";
        char v4[] = "./chcore_shell.bin";
        char v7[] = "./ext4.srv";
        char v18[] = "./test.config";
        char v22[] = "/client.bin";
        char v25[] = "/cat.bin";
        char v32[] = "wlan/Bmo0CxRi";

        open(v4, 2, 0);
        unlink(v22);
        symlink(v18, v32);
        rename(v25, v7);
        open(v2, 65536, 0);
        unlink(v7);
}

static void test3(void)
{
        char v37[] = "./eICgTeQz";
        int v38;
        v38 = open(v37, 66, 438);
        fallocate(v38, 25, 4755, 927);
}

static void test4(void)
{ // fill_statbuf
        char v1[8192];
        char v2[] = ".";
        char v15[] = "./liblidar_sdk.so";
        char v24[] = "test_dm.bin";
        long v30;

        v30 = open(v2, 65536, 0);
        rename(v15, v24);
        fstatat(v30, v24, (struct stat*)v1, 256);
}

void fuzzing_testsuit_tmpfs(void)
{
        test0();
        test1();
        test2();
        test3();
        test4();
}

int main(void)
{
        fuzzing_testsuit_tmpfs();
        return 0;
}
