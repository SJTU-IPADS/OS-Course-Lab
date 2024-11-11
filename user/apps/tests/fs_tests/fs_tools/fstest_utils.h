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

#pragma once

#define READ_NUM       10000
#define WRITE_NUM      100
#define NAME_LENGTH    100
#define THREA_NUM      40
#define PROCESS_NUM    20
#define MAX_FD         1024
#define TEST_FILE_SIZE (100 * PAGE_SIZE)

int random_write_test(char *addr, int offset, int size, int fd);
int random_read_test(char *addr, int offset, int size);
void deinit_file(int fd, char *fname);
int init_file(char *fname, int size);