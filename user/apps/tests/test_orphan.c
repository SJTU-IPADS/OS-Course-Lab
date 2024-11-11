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
#include <chcore/launcher.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <assert.h>

#define LEAF_NUM 3
#define ORPHAN_NUM 3

void leaf(void)
{
        printf("[leaf] Start\n");

        sleep(5);

        printf("[leaf] Exit\n");
}

void orphan(void)
{
        char *args[2];
        int pid[LEAF_NUM];
        int i, ret;

        printf("[orphan] Start\n");

        args[0] = "/test_orphan.bin";
        args[1] = "leaf";

        for (i = 0; i < LEAF_NUM; i++) {
                pid[i] = create_process(2, args, NULL);
                printf("[orphan] create leaf %d\n", pid[i]);
        }

        for (i = 0; i < LEAF_NUM; i++) {
                ret = waitpid(pid[i], NULL, 0);
                assert(ret == pid[i]);
                printf("[orphan] wait leaf %d\n", pid[i]);
        }
        
        sleep(2);

        printf("[orphan] Exit\n");
}

void root(void)
{
        char *args[2];
        int i;

        printf("[root] Creating orphans\n");

        args[0] = "/test_orphan.bin";
        args[1] = "orphan";
        
        for (i = 0; i < ORPHAN_NUM ; i++) {
                create_process(2, args, NULL);
        }

        printf("[root] Exit\n");
}

int main(int argc, char *argv[], char *envp[])
{
        if (argc == 2 && !strcmp(argv[1], "leaf")) {
                leaf();
        } else if (argc == 2 && !strcmp(argv[1], "orphan")) {
                orphan();
        } else {
                root();
        }
        return 0;
}
