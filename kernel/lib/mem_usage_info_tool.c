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

#include <lib/mem_usage_info_tool.h>

#include <common/backtrace.h>
#include <common/types.h>
#include <mm/kmalloc.h>


#ifdef CHCORE
#include <object/thread.h>
#include <object/cap_group.h>
#endif

#if ENABLE_MEMORY_USAGE_COLLECTING == ON
struct addr_to_size {
        vaddr_t addr;
        size_t size;
        size_t struct_real_size;
        vaddr_t next_ip;
        vaddr_t lr[20];
        vaddr_t fp[20];
        int backtrace_level;
        struct list_head node;
        struct list_head order_list_node;
};

struct memory_use_msg {
        char cap_group_name[64];
        bool valid;
        struct list_head addr_to_size_head;
} free_mem_msg[RECORD_SIZE];

static unsigned int collecting_tool_memory_usage = 0;
bool collecting_switch = false;
static struct list_head order_list_node_head;
struct lock record_lock;

struct memory_use_msg *get_record_slot(void)
{
        int i;
        struct memory_use_msg *msg = NULL;

        // NOTE: We do not consider recycling the record slot.
        for (i = 0; i < RECORD_SIZE; i++) {
                if ((!free_mem_msg[i].valid)
                    || strcmp(free_mem_msg[i].cap_group_name,
                              current_cap_group->cap_group_name)
                               == 0) {
                        msg = &(free_mem_msg[i]);
                        break;
                }
        }

        if (msg && !msg->valid) {
                memcpy(msg->cap_group_name,
                       current_cap_group->cap_group_name,
                       strlen(current_cap_group->cap_group_name));
                msg->valid = true;
        }

        return msg;
}

void record_mem_usage(size_t size, void *addr)
{
        struct memory_use_msg *msg;
        struct addr_to_size *new_record;
        size_t real_size;

        lock(&record_lock);
        msg = get_record_slot();

        if (!msg) {
                printk("There is no more space to store memory info\n");
                unlock(&record_lock);
                return;
        }

        new_record = _kmalloc(sizeof(struct addr_to_size), false, &real_size);
        memset(new_record, 0, sizeof(struct addr_to_size));
        new_record->addr = (vaddr_t)addr;
        new_record->size = size;
        new_record->struct_real_size = real_size;
        new_record->backtrace_level = set_backtrace_data(
                new_record->lr, new_record->fp, &new_record->next_ip);

        collecting_tool_memory_usage += real_size;

        list_add(&(new_record->order_list_node), &order_list_node_head);
        list_add(&(new_record->node), &msg->addr_to_size_head);
        unlock(&record_lock);
}

void revoke_mem_usage(void *addr)
{
        struct addr_to_size *record = NULL;
        bool find = false;

        lock(&record_lock);
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
                collecting_tool_memory_usage -= (record->struct_real_size);
                _kfree(record, false);
        }
        unlock(&record_lock);
}
#endif

void get_mem_usage_msg(void)
{
#if ENABLE_MEMORY_USAGE_COLLECTING == ON
        int i, j;
        struct addr_to_size *temp;
        unsigned int unfreed_memory = 0;

        if (unlikely(!collecting_switch)) {
                init_list_head(&order_list_node_head);
                for (i = 0; i < RECORD_SIZE; i++) {
                        init_list_head(&free_mem_msg[i].addr_to_size_head);
                }
                lock_init(&record_lock);
                collecting_switch = true;
        } else {
                lock(&record_lock);
                for (i = 0; i < RECORD_SIZE; i++) {
                        if (free_mem_msg[i].valid) {
                                printk("%s\n", free_mem_msg[i].cap_group_name);
                                for_each_in_list (
                                        temp,
                                        struct addr_to_size,
                                        node,
                                        &(free_mem_msg[i].addr_to_size_head)) {
                                        printk("--------------------------------------------------\n");
                                        printk("addr:0x%lx  size:0x%lx\n",
                                               temp->addr,
                                               temp->size);
                                        printk("backtrace msg:\n");
                                        for (j = 0; j < temp->backtrace_level;
                                             j++) {
                                                printk("ip : 0x%16lx fp : 0x%16lx\n",
                                                       temp->lr[j],
                                                       temp->fp[j]);
                                        }
                                        printk("next ip:0x%lx\n",
                                               temp->next_ip);
                                        unfreed_memory += temp->size;
                                }
                                printk("==================================================\n");
                        }
                }
                printk("Collecting tool usage : 0x%lx\n",
                       collecting_tool_memory_usage);
                unfreed_memory += collecting_tool_memory_usage;
                printk("Unfreed memory(include memory consumed by collecting) : 0x%lx\n",
                       unfreed_memory);
                unlock(&record_lock);
        }
#endif
}
