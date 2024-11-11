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

#define _GNU_SOURCE
#include <sched.h>
#include <uspi/devicenameservice.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <chcore/syscall.h>
#include <chcore/ipc.h>
#include <chcore/launcher.h>
#include <pthread.h>
#include <chcore-internal/usb_devmgr_defs.h>
#include <uvc_dispatch.h>
#include <serial_dispatch.h>
#include <errno.h>
#include <chcore/memory.h>
#include <chcore/string.h>
#include <uspi/devicenameservice.h>
#include <uspi/usbkeyboard.h>
#include <uspi/usbhid.h>
#include <uspi/keymap.h>
#include <uspios.h>
#include <uspi.h>

#include "ethernet.h"

#include <libpipe.h>

#define PERIPHERAL_BASE (0x3F000000UL)
#define PHYSMEM_END     (0x40000000UL)

void *car_demo_thread(void *arg)
{
	extern void carboard_controller(char cmd);
	carboard_controller((char)(u64)arg);

	usys_exit(0);
	return NULL;
}

static int keyboard_pipe_pmo = -1;
static struct pipe *keyboard_pipe = NULL;

#if 1
static void KeyStatusHandlerRaw(u8 ucModifiers, const u8 RawKeys[6])
{
	struct {
		u32 mods_depressed;
		u32 mods_latched;
		u32 mods_locked;
		u32 keys[6];
	} event = {0};
	TUSBKeyboardDevice *keyboard;
	u16 keysym;
	u8 key;
	u32 i;

	keyboard = DeviceNameServiceGetDevice(
		DeviceNameServiceGet(), "ukbd1", FALSE);
	if (keyboard == NULL)
		return;

	for (i = 7; i >= 2; i--) {
		key = keyboard->m_pReportBuffer[i];
		if (key != 0)
			break;
	}

	if (key == keyboard->m_ucLastPhyCode)
		key = 0;
	else
		keyboard->m_ucLastPhyCode = key;

	if (key != 0) {
		KeyMapTranslate(&keyboard->m_KeyMap, key, ucModifiers);
	} else if (keyboard->m_hTimer != 0) {
		CancelKernelTimer(keyboard->m_hTimer);
		keyboard->m_hTimer = 0;
	}

	if (keyboard_pipe == NULL)
		return;

	if (ucModifiers & (LSHIFT | RSHIFT))
		event.mods_depressed |= 1 << 0;
	if (ucModifiers & (LCTRL | RCTRL))
		event.mods_depressed |= 1 << 2;
	if (ucModifiers & (ALT | ALTGR))
		event.mods_depressed |= 1 << 3;

	if (keyboard->m_KeyMap.m_bCapsLock)
		event.mods_locked |= 1 << 1;
	if (keyboard->m_KeyMap.m_bNumLock)
		event.mods_locked |= 1 << 4;
	if (keyboard->m_KeyMap.m_bScrollLock)
		event.mods_locked |= 1 << 5;

	for (i = 0; i < 6; i++) {
		keysym = keyboard->m_KeyMap.m_KeyMap[RawKeys[i]][0];

		if (keysym == KeyCapsLock)
			event.mods_depressed |= 1 << 1;
		else if (keysym == KeyNumLock)
			event.mods_depressed |= 1 << 4;
		else if (keysym == KeyScrollLock)
			event.mods_depressed |= 1 << 5;

		event.keys[i] = RawKeys[i];
	}

	pipe_write_exact(keyboard_pipe, &event, sizeof(event));
}
#else
static void KeyPressedHandler(const char *pString)
{
	char cmd;
	pthread_t tid;

	cmd = pString[0];
	pthread_create(&tid, NULL, car_demo_thread, (void *)(u64)cmd);
}
#endif

static int mouse_pipe_pmo = -1;
static struct pipe *mouse_pipe = NULL;

static void MouseStatusHandler(u32 nButtons, int nDisplacementX,
			       int nDisplacementY, int nScrollVector)
{
	struct {
		u32 buttons;
		int x, y, scroll;
	} event;

	if (mouse_pipe == NULL)
		return;

	event.buttons = nButtons;
	event.x = nDisplacementX;
	event.y = nDisplacementY;
	event.scroll = nScrollVector;

	pipe_write_exact(mouse_pipe, &event, sizeof(event));
}

/*
 * Mapping peripheral memory.
 * FIXME: A simple but dangerous impletation now.
 */
static void setup_peripheral_mappings(void)
{
	int ret;
	cap_t pmo_cap;

/* Access [0 - 4G] physical/bus address */
#define SIZE_3G (3UL * 1024 * 1024 * 1024)
	pmo_cap = usys_create_device_pmo(
		PERIPHERAL_BASE, PHYSMEM_END - PERIPHERAL_BASE + SIZE_3G);
	// printf("pmo_cap is %d\n", pmo_cap);
	ret = usys_map_pmo(0, pmo_cap, PERIPHERAL_BASE, VM_READ | VM_WRITE);
	// printf("map pmo_cap ret %d\n", ret);
	assert(ret == 0);

	printf("%s done.\n", __func__);
}

/* Bind current thread to a specific CPU. */
static void bind_to_cpu(int cpu)
{
	int ret;
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	ret = sched_setaffinity(0, sizeof(mask), &mask);
	assert(ret == 0);
	usys_yield();
}

static int usb_open_device_handler(ipc_msg_t *ipc_msg, badge_t client_badge)
{
	int ret;
	TDeviceInfo *pInfo;
	enum USB_DEV_CLASS dev_class;
	int dev_id;
	struct usb_devmgr_request *req =
		(struct usb_devmgr_request *)ipc_get_msg_data(ipc_msg);

	/* Try to get device info according to the given name */
	pInfo = DeviceNameServiceGetInfo(DeviceNameServiceGet(), req->devname);
	if (pInfo == NULL) {
		return -ENODEV;
	}

	/* Try lock this device */
	ret = pthread_mutex_trylock(&pInfo->lock);
	if (ret) {
		/* The device has already been locked */
		if (req->wait) {
			/* Block until the mutex is available */
			pthread_mutex_lock(&pInfo->lock);
		} else {
			/* return the error condition */
			return -EBUSY;
		}
	}
	pInfo->owner_badge = client_badge;
	dev_class = pInfo->dev_class;
	dev_id = pInfo->dev_id;

	ipc_set_msg_data(ipc_msg, &dev_class, 0, sizeof(dev_class));
	ipc_set_msg_data(ipc_msg, &dev_id, sizeof(dev_class), sizeof(dev_id));
	printf("in %s set dev_class:%d, dev_id:%d.\n",
	       __func__,
	       dev_class,
	       dev_id);
	return 0;
}

static int usb_close_device_handler(ipc_msg_t *ipc_msg, badge_t client_badge)
{
	TDeviceInfo *pInfo;
	struct usb_devmgr_request *req =
		(struct usb_devmgr_request *)ipc_get_msg_data(ipc_msg);

	/* Try to get device info according to the given name */
	pInfo = DeviceNameServiceGetInfoByID(DeviceNameServiceGet(),
					     req->dev_id);

	if (pInfo == NULL) {
		return -ENXIO;
	}

	if (client_badge != pInfo->owner_badge) {
		return -EACCES;
	}

	pInfo->owner_badge = 0;
	pthread_mutex_unlock(&pInfo->lock);
	return 0;
}

static int usb_list_device_handler(ipc_msg_t *ipc_msg, badge_t client_badge)
{
	TDeviceInfo *pInfo;
	TDeviceInfoForClient pClientInfo;
	TDeviceNameService *pThis = DeviceNameServiceGet();
	DeviceNameServiceListAllDevice(pThis);
	u32 devnum;
	int cnt = 0;
	int offset = 0;

	if (pThis == 0) {
		printf("Empty DeviceNameService!\n");
		assert(0);
	}

	// devnum is the max num of device info this ipc_msg can contain
	devnum = *(u32 *)((u8 *)ipc_get_msg_data(ipc_msg)
			  + sizeof(struct usb_devmgr_request));
	pInfo = pThis->m_pList;

	while (pInfo && (cnt < devnum)) {
		pClientInfo.dev_class = pInfo->dev_class;
		strlcpy(pClientInfo.devname,
		        pInfo->pName,
		        sizeof(pClientInfo.devname));
		strlcpy(pClientInfo.vendor,
		        pInfo->pVendor,
		        sizeof(pClientInfo.vendor));
		ipc_set_msg_data(
			ipc_msg, &pClientInfo, offset, sizeof(pClientInfo));
		offset += sizeof(pClientInfo);
		cnt++;
		pInfo = pInfo->pNext;
	}

	while (pInfo) {
		cnt++;
		pInfo = pInfo->pNext;
	}

	// return the actual num of devices
	return cnt;
}

static int usb_get_device_service_handler(ipc_msg_t *ipc_msg, badge_t client_badge)
{
	struct usb_devmgr_request *req =
		(struct usb_devmgr_request *)ipc_get_msg_data(ipc_msg);
	int ret = 0;

	TDeviceInfo *pInfo = DeviceNameServiceGetInfoByID(
		DeviceNameServiceGet(), req->dev_id);

	if (pInfo->owner_badge != client_badge) {
		printf("Cannot get device which does not belong to you.\n");
		return -EACCES;
	}

	if (pInfo->dev_class != req->dev_class) {
		printf("Device class mismatch.\n");
		return -EINVAL;
	}

	switch (req->dev_class) {
	case USB_SERIAL:
		//printf("request serial service!\n");
		ret = serial_dispatch(ipc_msg, client_badge);
		break;
	case USB_UVC:
		printf("request uvc service!\n");
		ret = uvc_dispatch(ipc_msg, client_badge);
		break;
	case USB_STORAGE:
		printf("request storage service!\n");
		break;
	default:
		printf("[Devmgr] Unknown device class.\n");
		break;
	}

	return ret;
}

static void init_mouse_key_pipe(void)
{
	mouse_pipe_pmo = usys_create_pmo(PAGE_SIZE, PMO_DATA);
	mouse_pipe = create_pipe_from_pmo(PAGE_SIZE, mouse_pipe_pmo);
	pipe_init(mouse_pipe);

	keyboard_pipe_pmo = usys_create_pmo(PAGE_SIZE, PMO_DATA);
	keyboard_pipe = create_pipe_from_pmo(PAGE_SIZE, keyboard_pipe_pmo);
	pipe_init(keyboard_pipe);
}

static void usb_get_keyboard_pipe_handler(ipc_msg_t *ipc_msg, badge_t client_badge)
{
	ipc_set_msg_return_cap_num(ipc_msg, 1);
	ipc_set_msg_cap(ipc_msg, 0, keyboard_pipe_pmo);
	ipc_return_with_cap(ipc_msg, 0);
}

static void usb_get_mouse_pipe_handler(ipc_msg_t *ipc_msg, badge_t client_badge)
{
	ipc_set_msg_return_cap_num(ipc_msg, 1);
	ipc_set_msg_cap(ipc_msg, 0, mouse_pipe_pmo);
	ipc_return_with_cap(ipc_msg, 0);
}

DEFINE_SERVER_HANDLER(usb_devmgr_dispatch)
{
	struct usb_devmgr_request *req;
	int ret = 0;
	req = (struct usb_devmgr_request *)ipc_get_msg_data(ipc_msg);

	switch (req->req) {
	case USB_DEVMGR_LIST_DEV: {
		ret = usb_list_device_handler(ipc_msg, client_badge);
	} break;
	case USB_DEVMGR_OPEN: {
		ret = usb_open_device_handler(ipc_msg, client_badge);
	} break;
	case USB_DEVMGR_GET_DEV_SERVICE: {
		ret = usb_get_device_service_handler(ipc_msg, client_badge);
	} break;
	case USB_DEVMGR_CLOSE: {
		ret = usb_close_device_handler(ipc_msg, client_badge);
	} break;
	case USB_DEVMGR_GET_KEYBOARD_PIPE: {
		usb_get_keyboard_pipe_handler(ipc_msg, client_badge);
	} break;
	case USB_DEVMGR_GET_MOUSE_PIPE: {
		usb_get_mouse_pipe_handler(ipc_msg, client_badge);
	} break;
	default:
		break;
	}

	ipc_return(ipc_msg, ret);
}

int main(int argc, char *argv[], char *envp[])
{
	int ret;
	bool has_usb_support;
	bool has_usb_keyboard = false;

	setup_peripheral_mappings();
	bind_to_cpu(0);

#if 0
	void *token;
	/*
	 * A token for starting the procmgr server.
	 * This is just for preventing users run procmgr in the Shell.
	 * It is actually harmless to remove this.
	 */
	token = strstr(argv[0], KERNEL_SERVER);
	if (token == NULL) {
		printf("procmgr: I am a system server instead of an (Shell) "
		      "application. Bye!\n");
		usys_exit(-1);
	}
#endif

	/*
	 * Init the USPI driver framework.
	 * Note that there are another threads created in chcore-stub.c.
	 */
	init_mouse_key_pipe();
	ret = USPiInitialize();
	if (ret == 0) {
		has_usb_support = false;
		printf("No USB support.\n");
	} else {
		has_usb_support = true;
		printf("USB init done.\n");
	}

	if (has_usb_support) {
		ret = USPiKeyboardAvailable();
		if (ret == 0) {
			printf("No USB Keyboard.\n");
		} else {
			has_usb_keyboard = true;
			/* Enable Keyboard */
#if 1
			USPiKeyboardRegisterKeyStatusHandlerRaw(
				KeyStatusHandlerRaw);
#else
			USPiKeyboardRegisterKeyPressedHandler(
				KeyPressedHandler);
#endif
			printf("USB keyboard is enabled!\n");
		}

		ret = USPiMouseAvailable();
		if (ret == 0) {
			printf("No USB Mouse.\n");
		} else {
			USPiMouseRegisterStatusHandler(MouseStatusHandler);
			printf("USB mouse is enabled!\n");
		}

		/* Initialize ethernet driver */
		init_eth();

		printf("======================= USPI done =======================\n");
	}

	ipc_register_server(usb_devmgr_dispatch,
			    DEFAULT_CLIENT_REGISTER_HANDLER);

	if (has_usb_keyboard) {
		for (;;) {
			USPiKeyboardUpdateLEDs();
			usys_yield();
		}
	} else {
		for (;;) {
			usys_yield();
		}
	}

	return 0;
}
