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

/* usbch340.c added by ChCore. */

#include <uspi/usbch340.h>
#include <uspi/usbhostcontroller.h>
#include <uspi/devicenameservice.h>
#include <uspi/assert.h>
#include <uspi/usbhid.h>
#include <uspios.h>
#include <stdio.h>

static const char FromCH340[] = "CH340";

static unsigned s_nDeviceNumber = 0;

static int ch340_write_data(void *dev, void *buffer, int size);

void USBCH340Device(TUSBCH340Device *pThis, TUSBFunction *pDevice)
{
	assert(pThis != 0);

	USBFunctionCopy(&pThis->m_USBFunction, pDevice);
	pThis->m_USBFunction.Configure = USBCH340DeviceConfigure;

	pThis->m_pEndpointIn = 0;
	pThis->m_pEndpointOut = 0;
	pThis->m_USBSerialDriver.write = ch340_write_data;
}

void _CUSBCH340Device(TUSBCH340Device *pThis)
{
	assert(pThis != 0);

	if (pThis->m_pEndpointIn != 0) {
		_USBEndpoint(pThis->m_pEndpointIn);
		free(pThis->m_pEndpointIn);
		pThis->m_pEndpointIn = 0;
	}

	if (pThis->m_pEndpointOut != 0) {
		_USBEndpoint(pThis->m_pEndpointOut);
		free(pThis->m_pEndpointOut);
		pThis->m_pEndpointOut = 0;
	}

	_USBFunction(&pThis->m_USBFunction);
}

TUSBCH340Device *p_ch340;

void ch340_configure(TUSBCH340Device *pThis)
{
	p_ch340 = pThis;

	LogWrite(
		FromCH340, LOG_NOTICE, "func %s, line: %d", __func__, __LINE__);

	unsigned char buffer[8];
	int size = 8;
	int ret;

	for (int i = 0; i < size; ++i)
		buffer[i] = 0;

	// ch341_control_in(dev, request, value, index, buf, bufsize)
	// ch341_control_in(dev, 0x5f, 0, 0, buffer, size);

	//int DWHCIDeviceControlMessage (TDWHCIDevice *pThis, TUSBEndpoint *pEndpoint,
	//		u8 ucRequestType, u8 ucRequest, u16 usValue, u16 usIndex,
	//		void *pData, u16 usDataSize)

	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_IN | REQUEST_VENDOR,
		0x5f,
		0,
		0,
		buffer,
		size);

	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "[check] buffer out: %x, %x",
		 buffer[0],
		 buffer[1]);
	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "[ret = %d] func %s, line: %d",
		 ret,
		 __func__,
		 __LINE__);

	// ch341_control_out(dev, request, value, index)
	// ch341_control_out(dev, 0xa1, 0, 0);

	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_OUT | REQUEST_VENDOR,
		0xa1,
		0,
		0,
		0,
		0);

	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "[ret = %d] func %s, line: %d",
		 ret,
		 __func__,
		 __LINE__);

	/* ch341_set_baudrate */

	// ch341_control_out(dev, 0x9a, 0x1312, 0xcc03);
	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_OUT | REQUEST_VENDOR,
		0x9a,
		0x1312,
		0xcc03,
		0,
		0);
	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "[ret = %d] func %s, line: %d",
		 ret,
		 __func__,
		 __LINE__);

	// TODO (tmac): not sure
	if (!ret) {
		// ch341_control_out(dev, 0x9a, 0x0f2c, b);
		ret = DWHCIDeviceControlMessage(
			USBFunctionGetHost(&pThis->m_USBFunction),
			USBFunctionGetEndpoint0(&pThis->m_USBFunction),
			REQUEST_OUT | REQUEST_VENDOR,
			0x9a,
			0x0f2c,
			0x0008,
			0,
			0);
		LogWrite(FromCH340,
			 LOG_NOTICE,
			 "[ret = %d] func %s, line: %d",
			 ret,
			 __func__,
			 __LINE__);
	}

	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_IN | REQUEST_VENDOR,
		0x95,
		0x2518,
		0,
		buffer,
		size);

	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "[check] buffer out: %x, %x",
		 buffer[0],
		 buffer[1]);
	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "[ret = %d] func %s, line: %d",
		 ret,
		 __func__,
		 __LINE__);

	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_OUT | REQUEST_VENDOR,
		0x9a,
		0x2518,
		0x0050,
		0,
		0);
	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "[ret = %d] func %s, line: %d",
		 ret,
		 __func__,
		 __LINE__);

	/* ch341_get_status */
	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_IN | REQUEST_VENDOR,
		0x95,
		0x0706,
		0,
		buffer,
		size);

	/* expect 0xff 0xee */
	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "[check] buffer out: %x, %x",
		 buffer[0],
		 buffer[1]);
	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "[ret = %d] func %s, line: %d",
		 ret,
		 __func__,
		 __LINE__);

	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_OUT | REQUEST_VENDOR,
		0xa1,
		0x501f,
		0xd90a,
		0,
		0);
	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "[ret = %d] func %s, line: %d",
		 ret,
		 __func__,
		 __LINE__);

	/* ch341_set_baudrate */
	// ch341_control_out(dev, 0x9a, 0x1312, 0xcc03);
	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_OUT | REQUEST_VENDOR,
		0x9a,
		0x1312,
		0xcc03,
		0,
		0);
	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "[ret = %d] func %s, line: %d",
		 ret,
		 __func__,
		 __LINE__);

	// TODO (tmac): not sure
	if (!ret) {
		// ch341_control_out(dev, 0x9a, 0x0f2c, b);
		ret = DWHCIDeviceControlMessage(
			USBFunctionGetHost(&pThis->m_USBFunction),
			USBFunctionGetEndpoint0(&pThis->m_USBFunction),
			REQUEST_OUT | REQUEST_VENDOR,
			0x9a,
			0x0f2c,
			0x0008,
			0,
			0);
		LogWrite(FromCH340,
			 LOG_NOTICE,
			 "[ret = %d] func %s, line: %d",
			 ret,
			 __func__,
			 __LINE__);
	}

	/* ch341_set_handshake */

	// ch341_control_out(dev, 0x9a, 0x1312, 0xcc03);
	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_OUT | REQUEST_VENDOR,
		0xa4,
		~((1 << 5) | (1 << 6)),
		0,
		0,
		0);
	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "[ret = %d] func %s, line: %d",
		 ret,
		 __func__,
		 __LINE__);

	/* ch341_get_status */
	ret = DWHCIDeviceControlMessage(
		USBFunctionGetHost(&pThis->m_USBFunction),
		USBFunctionGetEndpoint0(&pThis->m_USBFunction),
		REQUEST_IN | REQUEST_VENDOR,
		0x95,
		0x0706,
		0,
		buffer,
		size);

	/* expect 0x9f 0xee */
	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "[check] buffer out: %x, %x",
		 buffer[0],
		 buffer[1]);
	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "[ret = %d] func %s, line: %d",
		 ret,
		 __func__,
		 __LINE__);

	// DWHCIDeviceTransfer (TDWHCIDevice *pThis, TUSBEndpoint *pEndpoint, void *pBuffer, unsigned nBufSize)

	// TODO: move the following to car demo app
	char write_buffer[64];
	for (int i = 0; i < 64; ++i) {
		write_buffer[i] = (char)i;
	}

	unsigned char query_topic[8] = {
		0xff, 0xfe, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff};

	for (int i = 0; i < sizeof(query_topic); ++i) {
		write_buffer[i] = query_topic[i];
	}

	LogWrite(FromCH340, LOG_NOTICE, "write data to ch340...");

	ret = DWHCIDeviceTransfer(USBFunctionGetHost(&pThis->m_USBFunction),
				  pThis->m_pEndpointOut,
				  write_buffer,
				  sizeof(query_topic));

	LogWrite(FromCH340,
		 LOG_NOTICE,
		 "write data to ch340 done [ret = %d]",
		 ret);
}

void ch340_write(char buffer[], unsigned size)
{
	DWHCIDeviceTransfer(USBFunctionGetHost(&p_ch340->m_USBFunction),
			    p_ch340->m_pEndpointOut,
			    buffer,
			    size);
}

static int ch340_write_data(void *dev, void *buffer, int size)
{
	if (buffer == NULL || size == 0) {
		return 0;
	}
	TUSBCH340Device *pThis = (TUSBCH340Device *)dev;
	DWHCIDeviceTransfer(USBFunctionGetHost(&pThis->m_USBFunction),
			    pThis->m_pEndpointOut,
			    buffer,
			    size);
	return 0;
}

boolean USBCH340DeviceConfigure(TUSBFunction *pUSBFunction)
{
	TUSBCH340Device *pThis = (TUSBCH340Device *)pUSBFunction;
	assert(pThis != 0);

	LogWrite(
		FromCH340, LOG_NOTICE, "func %s, line: %d", __func__, __LINE__);

	if (USBFunctionGetNumEndpoints(&pThis->m_USBFunction) < 1) {
		USBFunctionConfigurationError(&pThis->m_USBFunction, FromCH340);

		return FALSE;
	}

	LogWrite(
		FromCH340, LOG_NOTICE, "func %s, line: %d", __func__, __LINE__);

	TUSBEndpointDescriptor *pEndpointDesc;
	while ((pEndpointDesc =
			(TUSBEndpointDescriptor *)USBFunctionGetDescriptor(
				&pThis->m_USBFunction, DESCRIPTOR_ENDPOINT))
	       != 0) {
		if ((pEndpointDesc->bEndpointAddress & 0x80) == 0x80 // Input EP
		    && (pEndpointDesc->bmAttributes & 0x3F)
			       == 0x03) // Interrupt
		{
			LogWrite(FromCH340,
				 LOG_NOTICE,
				 "####### skip interrupt EP for now.");
		}

		else if ((pEndpointDesc->bEndpointAddress & 0x80)
				 == 0x0 // Output EP
			 && (pEndpointDesc->bmAttributes & 0x3F)
				    == 0x02) // Bulk EP
		{
			LogWrite(FromCH340,
				 LOG_NOTICE,
				 "func %s, line: %d. ##### find Output-Bulk EP",
				 __func__,
				 __LINE__);

			assert(pThis->m_pEndpointOut == 0);
			pThis->m_pEndpointOut = malloc(sizeof(TUSBEndpoint));
			assert(pThis->m_pEndpointOut != 0);

			// max output size: 64
			USBEndpoint2(
				pThis->m_pEndpointOut,
				USBFunctionGetDevice(&pThis->m_USBFunction),
				(TUSBEndpointDescriptor *)pEndpointDesc);
		}

		else if ((pEndpointDesc->bEndpointAddress & 0x80)
				 == 0x80 // Input EP
			 && (pEndpointDesc->bmAttributes & 0x3F)
				    == 0x02) // Bulk EP
		{
			LogWrite(FromCH340,
				 LOG_NOTICE,
				 "func %s, line: %d. ##### find Input-Bulk EP",
				 __func__,
				 __LINE__);

			assert(pThis->m_pEndpointIn == 0);
			pThis->m_pEndpointIn = malloc(sizeof(TUSBEndpoint));
			assert(pThis->m_pEndpointIn != 0);

			USBEndpoint2(
				pThis->m_pEndpointIn,
				USBFunctionGetDevice(&pThis->m_USBFunction),
				(TUSBEndpointDescriptor *)pEndpointDesc);
		}
	}

	if (pThis->m_pEndpointIn == 0) {
		LogWrite(FromCH340,
			 LOG_NOTICE,
			 "func %s, line: %d. ##### ConfigureError.",
			 __func__,
			 __LINE__);

		USBFunctionConfigurationError(&pThis->m_USBFunction, FromCH340);

		return FALSE;
	}

	LogWrite(
		FromCH340, LOG_NOTICE, "func %s, line: %d", __func__, __LINE__);

	if (!USBFunctionConfigure(&pThis->m_USBFunction)) {
		LogWrite(FromCH340, LOG_ERROR, "Cannot set interface");

		return FALSE;
	}

	LogWrite(
		FromCH340, LOG_NOTICE, "func %s, line: %d", __func__, __LINE__);

	TString DeviceName;
	String(&DeviceName);
	StringFormat(&DeviceName, "CH340-%u", s_nDeviceNumber++);
	DeviceNameServiceAddDevice(
		DeviceNameServiceGet(),
		StringGet(&DeviceName),
		StringGet(USBDeviceGetName(pThis->m_USBFunction.m_pDevice,
					   DeviceNameVendor)),
		pThis,
		FALSE,
		USB_SERIAL);

	_String(&DeviceName);

	ch340_configure(pThis);

	return TRUE;
}
