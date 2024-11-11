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
#include <uspi/usbhostcontroller.h>
#include <uspi/devicenameservice.h>
#include <uspi/usbconfigparser.h>
#include <uspi/assert.h>
#include <uspi/usbhid.h>
#include <uspios.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <chcore/syscall.h>

#define SKIP_BYTES(pDesc, nBytes) ((TUSBDescriptor *)((u8 *)(pDesc) + (nBytes)))

/* Pay attention to this address. */
#define TEMP_ISOC_BUF_VA 0x300000000000

static const char FromUVC[] = "UVC";

static unsigned s_nDeviceNumber = 0;

static void uvc_video_complete_debug(TUSBRequest *pURB, void *pParam,
				     void *pContext);

void USBUVCDevice(TUSBUVCDevice *pThis, TUSBFunction *pDevice)
{
	assert(pThis != 0);

	USBFunctionCopy(&pThis->m_USBFunction, pDevice);
	pThis->m_USBFunction.Configure = USBUVCDeviceConfigure;

	pThis->int_endpoint = 0;
	pThis->m_INT_URB = 0;
	pThis->m_pPacketBuffer = 0;
	pThis->status = 0;
	pThis->capture_image = 0;
}

void _CUSBUVCDevice(TUSBUVCDevice *pThis)
{
	assert(pThis != 0);

	if (pThis->m_pPacketBuffer != 0) {
		free(pThis->m_pPacketBuffer);
		pThis->m_pPacketBuffer = 0;
	}

	if (pThis->int_endpoint != 0) {
		_USBEndpoint(pThis->int_endpoint);
		free(pThis->int_endpoint);
		pThis->int_endpoint = 0;
	}

	if (pThis->isoc_endpoint != 0) {
		_USBEndpoint(pThis->isoc_endpoint);
		free(pThis->isoc_endpoint);
		pThis->isoc_endpoint = 0;
	}

	_USBRequest(pThis->m_INT_URB);
	_USBFunction(&pThis->m_USBFunction);
}

static int uvc_status_complete(TUSBRequest *urb, void *pParam, void *pContext)
{
	TUSBUVCDevice *dev = (TUSBUVCDevice *)pContext;
	LogWrite("[Zixuan]",
		 LOG_NOTICE,
		 "urb status in %s: %d\n",
		 __func__,
		 urb->m_bStatus);
	LogWrite("[Zixuan]",
		 LOG_NOTICE,
		 "uvc device status[0] : %02x\n",
		 dev->status[0]);
	//while(1);
	return 0;
}

boolean USBUVCDeviceConfigure(TUSBFunction *pUSBFunction)
{
	/******************************************** Initialize ********************************************/
	//printf("Hello from a uvc device\n");

	TUSBUVCDevice *pThis = (TUSBUVCDevice *)pUSBFunction;

	if (USBFunctionGetNumEndpoints(&pThis->m_USBFunction) < 1) {
		USBFunctionConfigurationError(&pThis->m_USBFunction, FromUVC);

		return FALSE;
	}

	// int num_altsetting = pUSBFunction->m_pInterfaceDesc->bAlternateSetting;
	// int interface_num = pUSBFunction->m_pInterfaceDesc->bInterfaceNumber;
	//USBDescriptorEnumerate(pUSBFunction->m_pConfigParser);

	TUSBConfigurationParser *parser = pUSBFunction->m_pConfigParser;
	parser->m_pNextPosition = parser->m_pBuffer;

	while (parser->m_pNextPosition < parser->m_pEndPosition) {
		u8 ucDescLen = parser->m_pNextPosition->Header.bLength;
		u8 ucDescType = parser->m_pNextPosition->Header.bDescriptorType;

		TUSBDescriptor *pDescEnd =
			SKIP_BYTES(parser->m_pNextPosition, ucDescLen);
		assert(pDescEnd <= parser->m_pEndPosition);

		if (ucDescType == DESCRIPTOR_INTERFACE) {
			TUSBInterfaceDescriptor *id =
				(TUSBInterfaceDescriptor
					 *)(parser->m_pNextPosition);
			if (id->bAlternateSetting == 4) {
				parser->m_pNextPosition = pDescEnd;
				ucDescType = parser->m_pNextPosition->Header
						     .bDescriptorType;
				assert(ucDescType == DESCRIPTOR_ENDPOINT);
				TUSBEndpointDescriptor *ep =
					(TUSBEndpointDescriptor
						 *)(parser->m_pNextPosition);
				pThis->isoc_endpoint = (TUSBEndpoint *)malloc(
					sizeof(TUSBEndpoint));
				USBEndpoint2(pThis->isoc_endpoint,
					     USBFunctionGetDevice(
						     &pThis->m_USBFunction),
					     (TUSBEndpointDescriptor *)ep);
				LogWrite(
					"[Zixuan]",
					LOG_NOTICE,
					"isoc_endpoint initialized, type: %02x, addr: %02x, maxPacketSize: %d, interval: %d",
					ep->bmAttributes,
					ep->bEndpointAddress,
					(ep->wMaxPacketSize),
					ep->bInterval);
			}
		} else if (ucDescType == DESCRIPTOR_ENDPOINT) {
			TUSBEndpointDescriptor *ep =
				(TUSBEndpointDescriptor
					 *)(parser->m_pNextPosition);
			if (ep->bEndpointAddress == 0x83) {
				pThis->int_endpoint = (TUSBEndpoint *)malloc(
					sizeof(TUSBEndpoint));
				USBEndpoint2(pThis->int_endpoint,
					     USBFunctionGetDevice(
						     &pThis->m_USBFunction),
					     (TUSBEndpointDescriptor *)ep);
				LogWrite(
					"[Zixuan]",
					LOG_NOTICE,
					"int_endpoint initialized, type: %02x, addr: %02x, maxPacketSize: %d, interval: %d",
					ep->bmAttributes,
					ep->bEndpointAddress,
					(ep->wMaxPacketSize),
					ep->bInterval);
			}
		}

		parser->m_pNextPosition = pDescEnd;
	}

	// Make sure that int_endpoint is successfully found.
	if (pThis->int_endpoint == 0) {
		LogWrite(FromUVC,
			 LOG_NOTICE,
			 "func %s, line: %d. ##### ConfigureError.",
			 __func__,
			 __LINE__);

		USBFunctionConfigurationError(&pThis->m_USBFunction, FromUVC);

		return FALSE;
	}

	// Make sure that isoc_endpoint is successfully found.
	if (pThis->isoc_endpoint == 0) {
		LogWrite(FromUVC,
			 LOG_NOTICE,
			 "func %s, line: %d. ##### ConfigureError.",
			 __func__,
			 __LINE__);

		USBFunctionConfigurationError(&pThis->m_USBFunction, FromUVC);

		return FALSE;
	}

	/******************************************** Configure ********************************************/
	int ret;
	/* Let interface 0 use altsetting 0 */
	if ((ret = DWHCIDeviceControlMessage(
		     USBFunctionGetHost(&pThis->m_USBFunction),
		     USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		     REQUEST_OUT | REQUEST_TO_INTERFACE,
		     SET_INTERFACE,
		     0,
		     0,
		     0,
		     0))
	    < 0) {
		LogWrite("[Zixuan]", LOG_ERROR, "Cannot set interface 0");

		return FALSE;
	}
	assert(ret == 0);

	unsigned char buffer[26];
	for (int i = 0; i < 26; i++) {
		buffer[i] = 0;
	}

	/************************ GET DEF ************************/
	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_IN | REQUEST_TO_INTERFACE | REQUEST_CLASS,
		UVC_GET_DEF,
		1 << 8,
		1,
		buffer,
		26);
	assert(ret == 26);
	// default: 00 00 01 01 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

	/************************ SET CUR ************************/
	// Some broken devices return null or wrong dwMaxVideoFrameSize
	buffer[18] = 0x4d;
	buffer[19] = 0x62;
	buffer[20] = 0x09;
	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_OUT | REQUEST_TO_INTERFACE | REQUEST_CLASS,
		UVC_SET_CUR,
		1 << 8,
		1,
		buffer,
		26);
	assert(ret == 0);

	/************************ GET CUR ************************/
	unsigned char cur_buffer[26];
	for (int i = 0; i < 26; i++) {
		cur_buffer[i] = 0;
	}
	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_IN | REQUEST_TO_INTERFACE | REQUEST_CLASS,
		UVC_GET_CUR,
		1 << 8,
		1,
		cur_buffer,
		26);
	assert(ret == 26);

	TString DeviceName;
	String(&DeviceName);
	StringFormat(&DeviceName, "UVC-%u", s_nDeviceNumber);
	DeviceNameServiceAddDevice(
		DeviceNameServiceGet(),
		StringGet(&DeviceName),
		StringGet(USBDeviceGetName(pThis->m_USBFunction.m_pDevice,
					   DeviceNameVendor)),
		pThis,
		FALSE,
		USB_UVC);
	_String(&DeviceName);

	pThis->data_pmo_cap = usys_create_pmo(3850 * 1600, PMO_DATA);
	pThis->isoc_buffer = TEMP_ISOC_BUF_VA;
	usys_map_pmo(0, pThis->data_pmo_cap, pThis->isoc_buffer, VM_WRITE | VM_READ);
	return TRUE;
}

int uvc_start_streaming(TUSBUVCDevice *pThis, u8 *buffer)
{
	/******************************************** Probing Camera Device ********************************************/
	int ret;

	// submit int_urb
	pThis->m_INT_URB = malloc(sizeof(TUSBRequest));
	pThis->status = malloc(UVC_MAX_STATUS_SIZE);
	pThis->status[0] = 0;
	USBRequest(pThis->m_INT_URB,
		   pThis->int_endpoint,
		   pThis->status,
		   UVC_MAX_STATUS_SIZE,
		   0);
	pThis->m_INT_URB->m_pCompletionContext = (void *)pThis;
	pThis->m_INT_URB->m_pCompletionRoutine =
		(TURBCompletionRoutine *)uvc_status_complete;
	DWHCIDeviceSubmitAsyncRequest(USBFunctionGetHost(&pThis->m_USBFunction),
				      pThis->m_INT_URB);

	/************************ SET CUR ************************/
	u8 set_cur_buf[26] = {0x01, 0x00, 0x01, 0x01, 0x15, 0x16, 0x05,
			      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			      0x00, 0x00, 0x00, 0x00, 0x00};
	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_OUT | REQUEST_TO_INTERFACE | REQUEST_CLASS,
		UVC_SET_CUR,
		1 << 8,
		1,
		set_cur_buf,
		26);
	assert(ret == 0);

	/************************ GET MIN ************************/
	u8 probe_min[26] = {0};
	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_IN | REQUEST_TO_INTERFACE | REQUEST_CLASS,
		UVC_GET_MIN,
		1 << 8,
		1,
		probe_min,
		26);
	assert(ret == 26);

	/************************ GET MAX ************************/
	u8 probe_max[26] = {0};
	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_IN | REQUEST_TO_INTERFACE | REQUEST_CLASS,
		UVC_GET_MAX,
		1 << 8,
		1,
		probe_max,
		26);
	assert(ret == 26);

	/************************ SET CUR ************************/
	u8 set_cur_buf1[26] = {0x01, 0x00, 0x01, 0x01, 0x15, 0x16, 0x5,
			       0x0,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			       0x00, 0x00, 0x00, 0x00, 0x00};
	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_OUT | REQUEST_TO_INTERFACE | REQUEST_CLASS,
		UVC_SET_CUR,
		1 << 8,
		1,
		set_cur_buf1,
		26);
	assert(ret == 0);

	/************************ GET CUR ************************/
	u8 get_cur_buf[26] = {0};
	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_IN | REQUEST_TO_INTERFACE | REQUEST_CLASS,
		UVC_GET_CUR,
		1 << 8,
		1,
		get_cur_buf,
		26);
	assert(ret == 26);

	/******************************************** Commit Stream Control ********************************************/
	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_OUT | REQUEST_TO_INTERFACE | REQUEST_CLASS,
		UVC_SET_CUR,
		UVC_VS_COMMIT_CONTROL << 8,
		1,
		get_cur_buf,
		26);
	assert(ret == 0);

	/******************************************** Start Streaming ********************************************/
	for (int i = 0; i < 26; i++) {
		printf("%02x ", get_cur_buf[i]);
	}

	printf("\n[Zixuan]  start streaming\n");
	u8 *isoc_buffer = (u8 *)pThis->isoc_buffer;

	u8 *picture_buffer =
		buffer; // should be big enough to contain a picture
	for (int i = 0; i < 3850 * 1600; i++) {
		isoc_buffer[i] = 0;
	}

	// make sure the buffer pointer is DWORD aligned
	assert(((u64)isoc_buffer & 0x3) == 0);

	TUSBRequest *uvc_urb = malloc(sizeof(TUSBRequest));
	USBRequestIsoc(uvc_urb,
		       pThis->isoc_endpoint,
		       isoc_buffer,
		       3850 * 1600,
		       0,
		       1600);
	uvc_urb->m_pCompletionContext = (void *)pThis;
	uvc_urb->m_pCompletionParam = picture_buffer;
	uvc_urb->m_pCompletionRoutine = uvc_video_complete_debug;

	/* Let interface 1 use altsetting 4*/
	if ((ret = DWHCIDeviceControlMessage(
		     USBFunctionGetHost(&pThis->m_USBFunction),
		     USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		     REQUEST_OUT | REQUEST_TO_INTERFACE,
		     SET_INTERFACE,
		     4,
		     1,
		     0,
		     0))
	    < 0) {
		printf("[Zixuan] Cannot set interface 1\n");
		while (1)
			;
	}
	assert(ret == 0);

	printf("submit urb\n");

	pThis->capture_image = TRUE;
	DWHCIDeviceSubmitIsocRequest(USBFunctionGetHost(&pThis->m_USBFunction),
				     uvc_urb);

	while (pThis->capture_image == TRUE)
		;

	_USBRequest(pThis->m_INT_URB);
	_USBRequest(uvc_urb);
	free(pThis->m_INT_URB);
	free(pThis->status);
	free(isoc_buffer);
	free(uvc_urb);
	printf("return value in %s: %d\n", __func__, pThis->image_actual_size);
	return pThis->image_actual_size;
}

int uvc_stop_streaming(TUSBUVCDevice *pThis)
{
	int ret;

	/* Let interface 1 use altsetting 0*/
	if ((ret = DWHCIDeviceControlMessage(
		     USBFunctionGetHost(&pThis->m_USBFunction),
		     USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		     REQUEST_OUT | REQUEST_TO_INTERFACE,
		     SET_INTERFACE,
		     0,
		     1,
		     0,
		     0))
	    < 0) {
		printf("[UVC] Cannot set interface 1 when stopping streaming.\n");
	}
	return ret;
}

static int error_count = 0;
static void uvc_video_complete_debug(TUSBRequest *pURB, void *pParam,
				     void *pContext)
{
	//printf("sucessfully enter uvc_video_complete_debug, start_addr: %x, actual len: %d\n", pURB->m_pBuffer, pURB->m_nResultLen);

	u8 *buf = (u8 *)(pURB->m_pBuffer);
	u8 *picture_buf = (u8 *)pParam;
	int byteused = 0;

	for (int i = 0; i < 3850; i++) {
		if (pURB->urb_isoc_frame_desc[i].status == 0) {
			assert(pURB->urb_isoc_frame_desc[i].actual_length >= 12
			       && pURB->urb_isoc_frame_desc[i].actual_length
					  <= 800);
		} else {
			error_count++;
		}

		if (pURB->urb_isoc_frame_desc[i].actual_length > 12) {
			memcpy(picture_buf + byteused,
			       buf + (pURB->urb_isoc_frame_desc[i].offset + 12),
			       pURB->urb_isoc_frame_desc[i].actual_length - 12);
			byteused +=
				pURB->urb_isoc_frame_desc[i].actual_length - 12;
		}

		if (buf[i * 1600 + 1] & (1 << 1)) {
			//frame_num++;
			// if (frame_num == 1000) {
			// 		struct timespec t;
			// 		int ret = clock_gettime(0, &t);
			// 		assert(ret == 0);
			// 		printf("[clock_gettime] t->secs: 0x%lx, t->nsecs: 0x%lx\n",
			// 			t.tv_sec, t.tv_nsec);
			// 	printf("1000 EOF with error count:%d\n", error_count);
			// 	while(1);
			// }
			printf("[Zixuan] EOF found, error_count:%d, actual_len:%d\n",
			       error_count,
			       byteused);
			TUSBUVCDevice *pThis = (TUSBUVCDevice *)pContext;
			pThis->image_actual_size = byteused;
			pThis->capture_image = FALSE;
			return;
		}
	}

	//printf("[Zixuan] decode begin\n");
	// for (int j = 0; j < byteused; j++) {
	// 	printf("%02X ", picture_buf[j]);
	// 	if ((j+1) % 50 == 0) {
	// 		printf("\n");
	// 	}
	// }
	//printf("\n[Zixuan] decode end\n");
	// TUSBUVCDevice *pThis = (TUSBUVCDevice *)pContext;
	// pURB->m_nResultLen = 0;
	// DWHCIDeviceSubmitIsocRequest(USBFunctionGetHost(&pThis->m_USBFunction), pURB);
}
