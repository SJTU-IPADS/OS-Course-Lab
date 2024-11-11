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

#include "defs.h"
#if DEBUG_MEM_USAGE
#include "chcore/container/list.h"
#include "chcore/type.h"
#include <stdio.h>
#include <stdlib.h>

bool collecting_switch = false;

struct memory_use_msg free_mem_msg[RECORD_SIZE];

char *types[RECORD_SIZE] = {"dentry", "string", "inode", "symlink", "data page"};

struct list_head order_list_node_head;

void tmpfs_get_mem_usage()
{
        struct addr_to_size *temp;
        unsigned long unfreed_memory = 0;

        if (!collecting_switch) {
                init_list_head(&order_list_node_head);
                for (int i = 0; i < RECORD_SIZE; i++) {
                        free_mem_msg[i].typename = types[i];
                        init_list_head(&free_mem_msg[i].addr_to_size_head);
                }
                collecting_switch = true;
        } else {
                for (int i = 0; i < RECORD_SIZE; i++) {
                        printf("%s\n", free_mem_msg[i].typename);
                        for_each_in_list (
                                temp,
                                struct addr_to_size,
                                node,
                                &(free_mem_msg[i].addr_to_size_head)) {
                                printf("--------------------------------------------\n");
                                printf("addr:0x%lx size:0x%lx\n",
                                       temp->addr,
                                       temp->size);
                                unfreed_memory += temp->size;
                        }
                        printf("============================================\n");
                }
                printf("unfreed memory used by tmpfs: 0x%lx\n", unfreed_memory);
        }
}

void tmpfs_record_mem_usage(void *addr, size_t size, int type)
{
        if (!collecting_switch) {
                return;
        }
        struct memory_use_msg *msg;
        struct addr_to_size *new_record;

        msg = &(free_mem_msg[type]);

        new_record = malloc(sizeof(struct addr_to_size));
        memset(new_record, 0, sizeof(struct addr_to_size));
        new_record->addr = (vaddr_t)addr;
        new_record->size = size;

        list_add(&(new_record->order_list_node), &order_list_node_head);
        list_add(&(new_record->node), &msg->addr_to_size_head);
}

void tmpfs_revoke_mem_usage(void *addr, int type)
{
        if (!collecting_switch) {
                return;
        }
        struct addr_to_size *record = NULL;
        bool find = false;

        for_each_in_list (record,
                          struct addr_to_size,
                          order_list_node,
                          &order_list_node_head) {
                if (record->addr == (vaddr_t)addr) {
                        find = true;
                        break;
                }
        }

        if (find) {
                list_del(&(record->node));
                list_del(&(record->order_list_node));
                free(record);
        }
}
#endif