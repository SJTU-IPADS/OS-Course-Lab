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
#include <stdlib.h>
#include <string.h>

#define BUFLEN 100

int main(int argc, char *argv[])
{
        FILE *f;
        FILE *f2;
        char buf[BUFLEN];
        char buf2[BUFLEN];
        int ret;

        if (argc != 3) {
                printf("Usage: diff [filename] [filename]\n");
                exit(-1);
        }
        f = fopen(argv[1], "r");
        if (f == NULL) {
                printf("%s:", argv[1]);
                perror("No such file or directory.\n");
                exit(-1);
        }
        f2 = fopen(argv[2], "r");
        if (f2 == NULL) {
                printf("%s:", argv[2]);
                perror("No such file or directory.\n");
                exit(-1);
        }
        memset(buf, 0, 100);
        memset(buf2, 0, 100);
        while (fgets((char *)buf, BUFLEN, f) != NULL) {
                if (fgets((char *)buf2, BUFLEN, f2) == NULL) {
                        printf("<%s", buf);
                }
                if ((ret = memcmp((void *)buf, (void *)buf2, BUFLEN)) != 0) {
                        printf("ret = %d\n", ret);
                        printf("<%s>%s", buf, buf2);
                }
                memset(buf, 0, 100);
                memset(buf2, 0, 100);
        }
        while (fgets((char *)buf2, BUFLEN, f2) != NULL) {
                printf(">%s", buf2);
        }
        return 0;
}
