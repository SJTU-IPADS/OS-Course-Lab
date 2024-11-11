//
// usbuvc.h
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright (C) 2016-2018  R. Stange <rsta2@o2online.de>
// Copyright (C) 2016  J. Otto <joshua.t.otto@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef _uspi_usbuvc_h
#define _uspi_usbuvc_h

#include <uspi/usbfunction.h>
#include <uspi/usbendpoint.h>
#include <uspi/usbrequest.h>
#include <uspi/types.h>
#include <chcore/type.h>

#define UVC_VS_PROBE_CONTROL  0x01
#define UVC_VS_COMMIT_CONTROL 0x02

#define UVC_MAX_STATUS_SIZE 16

#define UVC_SET_CUR  0x01
#define UVC_GET_CUR  0x81
#define UVC_GET_MIN  0x82
#define UVC_GET_MAX  0x83
#define UVC_GET_RES  0x84
#define UVC_GET_LEN  0x85
#define UVC_GET_INFO 0x86
#define UVC_GET_DEF  0x87

/* 2.4.3.3. Payload Header Information */
#define UVC_STREAM_EOH (1 << 7)
#define UVC_STREAM_ERR (1 << 6)
#define UVC_STREAM_STI (1 << 5)
#define UVC_STREAM_RES (1 << 4)
#define UVC_STREAM_SCR (1 << 3)
#define UVC_STREAM_PTS (1 << 2)
#define UVC_STREAM_EOF (1 << 1)
#define UVC_STREAM_FID (1 << 0)

typedef void TUVCPacketHandler(unsigned nCable, unsigned nLength, u8 *pPacket);

typedef struct TUSBUVCDevice {
	TUSBFunction m_USBFunction;

	TUSBRequest uvc_urb;
	TUSBEndpoint *int_endpoint;
	TUSBEndpoint *isoc_endpoint;

	TUSBRequest *m_INT_URB;
	u16 m_usBufferSize;
	u8 *m_pPacketBuffer;

	u8 *m_pReportBuffer;
	u8 *status;
	volatile boolean capture_image;
	volatile int image_actual_size;
	cap_t data_pmo_cap;
	u64 isoc_buffer;
} TUSBUVCDevice;

typedef struct TUVC_USBRequest {
} TUVC_USBRequest;

struct usb_iso_packet_descriptor {
	unsigned int offset;
	unsigned int length; /* expected length */
	unsigned int actual_length;
	int status;
};

void USBUVCDevice(TUSBUVCDevice *pThis, TUSBFunction *pFunction);
void _CUSBUVCDevice(TUSBUVCDevice *pThis);

boolean USBUVCDeviceConfigure(TUSBFunction *pUSBFunction);
int uvc_start_streaming(TUSBUVCDevice *pThis, u8 *buffer);
int uvc_stop_streaming(TUSBUVCDevice *pThis);
#endif
