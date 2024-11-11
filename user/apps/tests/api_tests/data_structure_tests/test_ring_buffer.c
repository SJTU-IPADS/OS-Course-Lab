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

#include <chcore/ring_buffer.h>
#include <stdio.h>

int main()
{
        struct ring_buffer *ring_buf;
        int msg = 1;

        ring_buf = new_ringbuffer(10, sizeof(int));
        if (!ring_buf)
                return -1;
        printf("new_ringbuffer success.\n");
        while (1) {
                set_one_msg(ring_buf, &msg);
                if (if_buffer_full(ring_buf)) {
                        printf("Buffer full!\n");
                        break;
                }
                msg++;
        }
        for (int i = 0; i < 9; i++) {
                get_one_msg(ring_buf, &msg);
                printf("get_one_msg: %d\n", msg);
        }
        free_ringbuffer(ring_buf);
        printf("free_ringbuffer success.\n");
        printf("Test finish!\n");
        return 0;
}
