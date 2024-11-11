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

int main(int argc, char *argv[])
{
        FILE *f;
        char buf[100];
        if (argc != 2) {
                printf("Usage: cat [filename]\n");
                exit(-1);
        }
        f = fopen(argv[1], "r");
        if (f == NULL) {
                perror("fopen err:\n");
                exit(-1);
        }
        memset(buf, 0, 100);
        while (fgets((char *)buf, 100, f) != NULL) {
                printf("%s", buf);
                memset(buf, 0, 100);
        }
        fclose(f);
        return 0;
}
