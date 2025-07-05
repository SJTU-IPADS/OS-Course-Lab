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
#include <sys/mman.h>
#include <string.h>
#include <chcore/defs.h>

/*
 * next_slot returns the virtual address of the next slot to the given offset.
 * This function should not be used by user directly, so we will not check the
 * correctness of the offset here.
 */
static inline off_t next_slot(struct ring_buffer *ring_buf, off_t off)
{
        return ((off == ring_buf->buffer_size - ring_buf->msg_size) ?
                        (sizeof(struct ring_buffer)) :
                        (off + ring_buf->msg_size));
}

int get_one_msg(struct ring_buffer *ring_buf, void *msg)
{
        off_t p_off = ring_buf->producer_offset;
        off_t c_off = ring_buf->consumer_offset;

        /* recycle_msg_buffer is empty */
        if (p_off == c_off)
                return MSG_OP_FAILURE;
        vaddr_t buf = (vaddr_t)ring_buf + c_off;
        memcpy(msg, (void *)buf, ring_buf->msg_size);
        ring_buf->consumer_offset = next_slot(ring_buf, c_off);
        return MSG_OP_SUCCESS;
}

/*
 * Whether ring buffer is full
 * Always keep a slot empty
 * Max capicity = buffer_size - msg_size
 * p_off = c_off                : buffer empty
 * c_off = next_slot(p_off)     : buffer full
 */
int if_buffer_full(struct ring_buffer *ring_buf)
{
        off_t p_off = ring_buf->producer_offset;
        off_t c_off = ring_buf->consumer_offset;

        /* recycle_msg_buffer is full */
        if (c_off == next_slot(ring_buf, p_off))
                return RING_BUFFER_FULL;

        return RING_BUFFER_NOT_FULL;
}

int set_one_msg(struct ring_buffer *ring_buf, void *msg)
{
        if (if_buffer_full(ring_buf)) {
                return MSG_OP_FAILURE;
        }

        off_t p_off = ring_buf->producer_offset;
        vaddr_t buf = (vaddr_t)ring_buf + p_off;

        memcpy((void *)buf, msg, ring_buf->msg_size);
        ring_buf->producer_offset = next_slot(ring_buf, p_off);
        return MSG_OP_SUCCESS;
}

struct ring_buffer *new_ringbuffer(int msg_num, size_t msg_size)
{
        if (msg_num <= 0) {
                return NULL;
        }

        size_t buffer_size = msg_num * msg_size + sizeof(struct ring_buffer);
        int page_num = ROUND_UP(buffer_size, 0x1000);
        struct ring_buffer *ring_buf =
                (struct ring_buffer *)mmap(NULL,
                                           page_num * 0x1000,
                                           PROT_READ | PROT_WRITE,
                                           MAP_ANONYMOUS | MAP_PRIVATE,
                                           -1,
                                           0);
        // struct ring_buffer *ring_buf = (struct ring_buffer
        // *)malloc(buffer_size);
        if (ring_buf == 0)
                return NULL;
        memset(ring_buf, 0, buffer_size);
        ring_buf->msg_size = msg_size;
        ring_buf->buffer_size = buffer_size;
        ring_buf->producer_offset = (off_t)sizeof(struct ring_buffer);
        ring_buf->consumer_offset = (off_t)sizeof(struct ring_buffer);
        return ring_buf;
}

void free_ringbuffer(struct ring_buffer *ring_buf)
{
        int page_num = ROUND_UP(ring_buf->buffer_size, 0x1000);
        munmap(ring_buf, page_num * 0x1000);
}