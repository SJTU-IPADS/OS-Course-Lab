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

#include <uspi/usbch340.h>
#include <uspi/usbcp210x.h>
#include <uspi/assert.h>
#include <uspi/usbserial.h>
#include <uspi/devicenameservice.h>
#include <serial_dispatch.h>
#include <chcore-internal/usb_devmgr_defs.h>
#include <stdio.h>
#include <errno.h>

static int serial_write(TUSBSerialDevice *serial_device, void *data,
			int datalen)
{
	if (serial_device->m_USBSerialDriver.write == NULL) {
		printf("[Serial] Requested device does not support serial write.\n");
		return -EINVAL;
	}

	return serial_device->m_USBSerialDriver.write(
		serial_device, data, datalen);
}

static int serial_read(TUSBSerialDevice *serial_device, void *data, int datalen)
{
	if (serial_device->m_USBSerialDriver.read == NULL) {
		printf("[Serial] Requested device does not support serial read.\n");
		return -EINVAL;
	}

	return serial_device->m_USBSerialDriver.read(
		serial_device, data, datalen);
}

static int serial_open(TUSBSerialDevice *serial_device)
{
	if (serial_device->m_USBSerialDriver.open == NULL) {
		printf("[Serial] Requested device does not support serial open.\n");
		return -EINVAL;
	}

	return serial_device->m_USBSerialDriver.open(serial_device);
}

static int serial_close(TUSBSerialDevice *serial_device)
{
	if (serial_device->m_USBSerialDriver.close == NULL) {
		printf("[Serial] Requested device does not support serial close.\n");
		return -EINVAL;
	}

	return serial_device->m_USBSerialDriver.close(serial_device);
}

static int serial_set_termios(TUSBSerialDevice *serial_device)
{
	if (serial_device->m_USBSerialDriver.set_termios == NULL) {
		printf("[Serial] Requested device does not support serial set_termios.\n");
		return -EINVAL;
	}

	serial_device->m_USBSerialDriver.set_termios(serial_device);
	return 0;
}

static int serial_get_termios(TUSBSerialDevice *serial_device)
{
	if (serial_device->m_USBSerialDriver.get_termios == NULL) {
		printf("[Serial] Requested device does not support serial get_termios.\n");
		return -EINVAL;
	}

	serial_device->m_USBSerialDriver.get_termios(serial_device);
	return 0;
}

static int serial_tiocmget(TUSBSerialDevice *serial_device)
{
	if (serial_device->m_USBSerialDriver.tiocmget == NULL) {
		printf("[Serial] Requested device does not support serial tiocmget.\n");
		return -EINVAL;
	}

	return serial_device->m_USBSerialDriver.tiocmget(serial_device);
}

static int serial_tiocmset(TUSBSerialDevice *serial_device, int set, int clear)
{
	if (serial_device->m_USBSerialDriver.tiocmset == NULL) {
		printf("[Serial] Requested device does not support serial tiocmset.\n");
		return -EINVAL;
	}

	return serial_device->m_USBSerialDriver.tiocmset(
		serial_device, set, clear);
}

static int serial_dtr_rts(TUSBSerialDevice *serial_device, int on)
{
	if (serial_device->m_USBSerialDriver.dtr_rts == NULL) {
		printf("[Serial] Requested device does not support serial dtr_rts.\n");
		return -EINVAL;
	}

	return serial_device->m_USBSerialDriver.dtr_rts(serial_device, on);
}

int serial_dispatch(ipc_msg_t *ipc_msg, badge_t client_badge)
{
	TUSBSerialDevice *serial_device;
	TDeviceInfo *pInfo;
	struct usb_devmgr_request *req;
	u64 ret = 0;
	void *buffer;
	int set, clear, on;

	req = (struct usb_devmgr_request *)ipc_get_msg_data(ipc_msg);

	/* Sanity check */
	assert(req->req == USB_DEVMGR_GET_DEV_SERVICE);
	assert(req->dev_class == USB_SERIAL);

	/* Get device info according to dev id */
	pInfo = DeviceNameServiceGetInfoByID(DeviceNameServiceGet(),
					     req->dev_id);
	if (pInfo == NULL) {
		printf("[Serial] Invalid device id.");
		return -EINVAL;
	}

	/* Get serial driver */
	serial_device = (TUSBSerialDevice *)pInfo->pDevice;
	assert(serial_device);

	buffer = ipc_get_msg_data(ipc_msg) + sizeof(struct usb_devmgr_request);

	switch (req->serial_req.req) {
	case SERIAL_WRITE:
		ret = serial_write(
			serial_device, buffer, req->serial_req.datalen);
		break;
	case SERIAL_READ:
		ret = serial_read(
			serial_device, buffer, req->serial_req.datalen);
		break;
	case SERIAL_OPEN:
		ret = serial_open(serial_device);
		break;
	case SERIAL_CLOSE:
		ret = serial_close(serial_device);
		break;
	case SERIAL_SET_TERMIOS:
		ret = serial_set_termios(serial_device);
		break;
	case SERIAL_GET_TERMIOS:
		ret = serial_get_termios(serial_device);
		break;
	case SERIAL_TIOCMGET:
		ret = serial_tiocmget(serial_device);
		break;
	case SERIAL_TIOCMSET:
		set = req->serial_req.set;
		clear = req->serial_req.clear;
		ret = serial_tiocmset(serial_device, set, clear);
		break;
	case SERIAL_DTR_RTS:
		on = req->serial_req.on;
		ret = serial_dtr_rts(serial_device, on);
		break;
	default:
		printf("[Serial] Unsupported request type.\n");
		break;
	}

	return ret;
}