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

#include <lib/ring_buffer.h>
#include <common/util.h>

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