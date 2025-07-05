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

#include <chcore/type.h>
#include <sys/types.h>

#define RING_BUFFER_FULL     1
#define RING_BUFFER_NOT_FULL 0
#define MSG_OP_SUCCESS       1
#define MSG_OP_FAILURE       0

/*
 * Ring buffer struct layout
 * buffer_size:                         size_t, size of whole struct including
 * the meta data consumer_offset, producer_offset:    int msg_size: size_t, size
 * of the msg that will be stored in the buffer There is also a data buffer to
 * actually store data, closely after the meta data above.
 */

struct ring_buffer {
        size_t buffer_size;
        off_t consumer_offset;
        off_t producer_offset;
        size_t msg_size;
};

int get_one_msg(struct ring_buffer *ring_buf, void *msg);
int set_one_msg(struct ring_buffer *ring_buf, void *msg);
int if_buffer_full(struct ring_buffer *ring_buf);
struct ring_buffer *new_ringbuffer(int msg_num, size_t msg_size);
void free_ringbuffer(struct ring_buffer *ring_buf);
