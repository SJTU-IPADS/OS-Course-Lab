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
#include <chcore/type.h>
#include <chcore/container/list.h>

#define BUFLEN                 4096
#define MAX_HISTORY_CMD_RECORD 10

extern cap_t fsm_scan_pmo_cap;
extern void *fsm_scan_buf;

/* fsm_server_cap in current process; can be copied to others */
extern cap_t fsm_server_cap;
/* lwip_server_cap in current process; can be copied to others */
extern cap_t lwip_server_cap;

extern bool waitchild;

struct history_cmd_node {
        char *cmd;
        int index;
        struct list_head cmd_node;
};
extern struct history_cmd_node *history_cmd_pointer;

void init_buildin_cmd(void);

void add_cmd_to_history(char *cmd);
void clear_history_point(void);

int do_complement(char *buf, char *complement, int complement_time);
int do_cd(char *cmdline);
int do_top(void);
int do_ls(char *cmdline);
void do_clear(void);
void do_jobs(void);
int do_fg(char *cmdline);
void do_history(void);
bool do_up(void);
bool do_down(void);