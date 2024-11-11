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

#include "mbox.h"

#include <chcore/memory.h>
#include <string.h>

u64 mbox_pa;
cap_t mbox_cap;
struct chcore_dma_handle mbox_msg_hndl;

#define MBOX_MSG_SIZE (36 * sizeof(u32))

/* Init mbox message buffer */
void mbox_init(void)
{
	void *mbox_msg = chcore_alloc_dma_mem(MBOX_MSG_SIZE, &mbox_msg_hndl);
	memset(mbox_msg, 0, MBOX_MSG_SIZE);
}

/*
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
static int mbox_call(u8 channel)
{
	u32 *mbox_msg = (u32 *)mbox_msg_hndl.vaddr;
	u32 r = (((u32)mbox_msg_hndl.paddr & ~0xF) | (channel & 0xF));

	/* Wait until we can write to the mailbox */
	do {asm volatile("nop");} while(*MBOX_STATUS & MBOX_FULL);

	/* Write the address of our message to the mailbox with channel identifier */
	*MBOX_WRITE = r;

	/* Now wait for the response */
	while(1) {
		/* Is there a response? */
		do {asm volatile("nop");} while(*MBOX_STATUS & MBOX_EMPTY);

		/* Is it a response to our message? */
		if (r == *MBOX_READ) {
			/* Is it a valid successful response? */
			return mbox_msg[1] == MBOX_RESPONSE;
		}
	}
	return 0;
}

int mbox_tag_msg(u32 *response_buf, // Pointer to response buffer
	u32 data_count, // Number of u32 data following
	u32 *args) // Variadic u32 values for call
{
	volatile u32 *mbox_msg = (u32 *)mbox_msg_hndl.vaddr;

	mbox_msg[0] = (data_count + 3) * 4; // Size of message needed
	mbox_msg[data_count + 2] = 0; // Set end pointer to zero
	mbox_msg[1] = 0; // Zero response message
	for (int i = 0; i < data_count; i++) {
		mbox_msg[2 + i] = args[i]; // Fetch next variadic
	}
	mbox_msg[2 + data_count] = MBOX_TAG_LAST;

	if (mbox_call(MBOX_CHANNEL_TAGS)) { // Wait for write response
		if (response_buf) { // If buffer NULL used then don't want response
			for (int i = 0; i < data_count; i++)
				response_buf[i] = mbox_msg[2 + i]; // Transfer out each response message
		}
		return 1; // Message success
	}
	return 0; // Message failed
}
