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

#include <pthread.h>
#include <stdio.h>
#include <bits/errno.h>
#include <chcore/ipc.h>
#include <chcore/syscall.h>
#include <chcore-internal/hdmi_defs.h>

#include "mbox.h"


extern u64 mbox_pa;
extern cap_t mbox_cap;
static pthread_mutex_t mbox_lock;


static void init_hdmi(void)
{
	/* Create and map mbox pmo */
	mbox_pa = VIDEOCORE_MBOX;
	mbox_cap = usys_create_device_pmo(mbox_pa, PAGE_SIZE);
	if (usys_map_pmo(SELF_CAP, mbox_cap, mbox_pa, 
		VM_READ | VM_WRITE) != 0) {
		printf("[HDMI] mbox map fail: va=0x%lx\n", mbox_pa);
		return;
	}

	/* Init mbox lock */
	pthread_mutex_init(&mbox_lock, NULL);

	/* Init mbox */
	mbox_init();
}

static int handle_get_fb(ipc_msg_t *ipc_msg, struct hdmi_request *hr)
{
	u32 resp_buf[23];
	u32 args[36];
	int width, height, depth;
	u64 fb_pa;
	int pitch;
	cap_t fb_cap;

	width = hr->width;
	height = hr->height;
	depth = hr->depth;
	
	/* Get width and height */
	if ((width == 0) || (height == 0)) { // Has auto width or heigth been requested
		args[0] = MBOX_TAG_GET_PHYSICAL_WIDTH_HEIGHT;
		args[1] = 8;
		args[2] = 0;
		args[3] = 0;
		args[4] = 0;
		if (mbox_tag_msg(&resp_buf[0], 5, args)) { // Get current width and height of screen
			if (width == 0)
				width = resp_buf[3]; // width passed in as zero set set current screen width
			if (height == 0)
				height = resp_buf[4]; // height passed in as zero set set current screen height
		}
		else {
			printf("[HDMI] get width height fail\n");
			return -1;
		} // For some reason get screen physical failed
	}

	/* Get depth */
	if (depth == 0) {
		args[0] = MBOX_TAG_GET_COLOUR_DEPTH;
		args[1] = 4;
		args[2] = 4;
		args[3] = 0; // Has auto colour depth been requested
		if (mbox_tag_msg(&resp_buf[0], 4, args)) { // Get current colour depth of screen
			depth = resp_buf[3]; // depth passed in as zero set set current screen colour depth
		}
		else {
			printf("[HDMI] get color depth fail\n");
			return -1;
		} // For some reason get screen depth failed
	}

	/* Get framebuffer and pitch */
	args[0] = MBOX_TAG_SET_PHYSICAL_WIDTH_HEIGHT;
	args[1] = 8;
	args[2] = 8;
	args[3] = width;
	args[4] = height;

	args[5] = MBOX_TAG_SET_VIRTUAL_WIDTH_HEIGHT;
	args[6] = 8;
	args[7] = 8;
	args[8] = width;
	args[9] = height;

	args[10] = MBOX_TAG_SET_COLOUR_DEPTH;
	args[11] = 4;
	args[12] = 4;
	args[13] = depth;

	args[14] = MBOX_TAG_ALLOCATE_FRAMEBUFFER;
	args[15] = 8;
	args[16] = 4;
	args[17] = 16;
	args[18] = 0;

	args[19] = MBOX_TAG_GET_PITCH;
	args[20] = 4;
	args[21] = 0;
	args[22] = 0;

	if (!mbox_tag_msg(&resp_buf[0], 23, args)) {
		printf("[HDMI] mailbox request fail\n");
		return -1;
	}

	fb_pa = resp_buf[17] & 0x3FFFFFFF; // Convert GPU address to ARM address
	pitch = resp_buf[22];

	/* Write ret value to hr */
	hr->ret_width = width;
	hr->ret_height = height;
	hr->ret_depth = depth;
	hr->ret_pitch = pitch;

	/* Create and set return framebuffer pmo cap */
	fb_cap = usys_create_device_pmo(fb_pa, width * height * (depth / 8));
	if (fb_cap < 0) {
		printf("[HDMI] create framebuffer pmo fail\n");
	}
	ipc_set_msg_return_cap_num(ipc_msg, 1);
	// ipc_set_ret_msg_cap(ipc_msg, fb_cap);
	ipc_set_msg_cap(ipc_msg, 0, fb_cap);

	return 0;
}

DEFINE_SERVER_HANDLER(hdmi_dispatch)
{
	int ret;
	struct hdmi_request *hr;

	hr = (struct hdmi_request *)ipc_get_msg_data(ipc_msg);

	if (hr->req == HDMI_GET_FB) {
		pthread_mutex_lock(&mbox_lock);
		ret = handle_get_fb(ipc_msg, hr);
		pthread_mutex_unlock(&mbox_lock);
		ipc_return_with_cap(ipc_msg, ret);
	}

	printf("[HDMI] A bad request!\n");
	ret = -EBADRQC;
	ipc_return(ipc_msg, ret);
}

int main(int argc, char *argv[], char *envp[])
{
	int ret;

	/* Init HDMI */
	init_hdmi();

	/* Register client handler */
	ret = ipc_register_server(hdmi_dispatch, 
			DEFAULT_CLIENT_REGISTER_HANDLER);
	printf("[HDMI] register server value = %d\n", ret);

	usys_wait(usys_create_notifc(), true, NULL);
	return 0;
}
