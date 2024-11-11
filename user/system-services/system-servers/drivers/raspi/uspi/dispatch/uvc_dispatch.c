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

#include <uspi/usbuvc.h>
#include <uspi/devicenameservice.h>
#include <pthread.h>
#include <uspi/assert.h>
#include <stdlib.h>
#include <chcore/ipc.h>
#include <chcore/syscall.h>
#include <chcore-internal/usb_devmgr_defs.h>
#include <uvc_dispatch.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>

static int uvc_capture_image(TUSBUVCDevice *pThis, int shm_key)
{
	u64 ret;
	int shmid;
	void *shmaddr;

	assert(pThis != NULL);

	// prepare shared memory
	shmid = shmget(shm_key, JPEG_IMG_BUF_SIZE, IPC_CREAT);
	shmaddr = shmat(shmid, 0, 0);

	memset(shmaddr, 0, JPEG_IMG_BUF_SIZE);

	/*
	 * The jpeg bytes will be written to the shared memory.
	 * The actual len of the jpeg imgae will be returned.
	 */
	ret = uvc_start_streaming(pThis, (u8 *)shmaddr);
	uvc_stop_streaming(pThis);
	shmdt(shmaddr);
	return ret;
}

int uvc_dispatch(ipc_msg_t *ipc_msg, badge_t client_badge)
{
	TUSBUVCDevice *uvc_device;
	TDeviceInfo *pInfo;
	struct usb_devmgr_request *req;
	u64 ret = 0;

	req = (struct usb_devmgr_request *)ipc_get_msg_data(ipc_msg);

	/* Sanity check */
	assert(req->req == USB_DEVMGR_GET_DEV_SERVICE);
	assert(req->dev_class == USB_UVC);

	/* Get device info according to dev id */
	pInfo = DeviceNameServiceGetInfoByID(DeviceNameServiceGet(),
					     req->dev_id);
	if (pInfo == NULL) {
		printf("[UVC] Invalid device id.");
		return -EINVAL;
	}
	uvc_device = pInfo->pDevice;
	assert(uvc_device);

	switch (req->uvc_req.req) {
	case UVC_CAPTURE_IMG:
		ret = uvc_capture_image(uvc_device, req->uvc_req.shm_key);
		break;
	default:
		printf("[UVC] Invalid request type.\n");
		break;
	}
	return ret;
}
