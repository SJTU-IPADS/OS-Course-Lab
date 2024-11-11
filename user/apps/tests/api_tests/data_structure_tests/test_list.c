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

#include <chcore/container/list.h>
#include <stdio.h>

int main()
{
        struct list_head list, node1, node2, node3;
        init_list_head(&list);
        printf("init_list_head success.\n");
        if (list_empty(&list))
                printf("List is empty.\n");
        else {
                printf("List is not empty!\n");
                return -1;
        }
        list_add(&node1, &list);
        printf("list_add success.\n");
        list_append(&node2, &list);
        printf("list_append success.\n");
        list_add(&node3, &list);
        if (!list_empty(&list))
                printf("List is not empty.\n");
        else {
                printf("List is empty!\n");
                return -1;
        }
        list_del(&node1);
        printf("list_del success.\n");
        printf("Test finish!\n");
        return 0;
}