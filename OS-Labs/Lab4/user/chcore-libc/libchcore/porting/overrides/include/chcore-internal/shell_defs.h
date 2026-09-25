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

#define SHELL_REQ_BUFSIZE 1024

enum SHELL_REQ {
        SHELL_SET_PROCESS_INFO = 1,
        SHELL_APPEND_INPUT_BUFFER,
        SHELL_SET_FG_PROCESS,
        SHELL_FG_PROCESS_RETURN
};

struct shell_req {
        int req;
        pid_t pid;
        char buf[SHELL_REQ_BUFSIZE];
        size_t size;
};

void shell_new_fg_proc(int pid);

void shell_fg_proc_return(int pid);
