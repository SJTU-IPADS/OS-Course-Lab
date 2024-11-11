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

/* usbch340.h added by ChCore */

#ifndef _uspi_usbch340_h
#define _uspi_usbch340_h

#include <uspi/usbfunction.h>
#include <uspi/usbendpoint.h>
#include <uspi/usbrequest.h>
#include <uspi/types.h>
#include <uspi/usbserial.h>

typedef struct TUSBCH340Device {
	TUSBFunction m_USBFunction;
	TUSBSerialDriver m_USBSerialDriver;
	TUSBEndpoint *m_pEndpointIn;
	TUSBEndpoint *m_pEndpointOut;
} TUSBCH340Device;

void USBCH340Device(TUSBCH340Device *pThis, TUSBFunction *pFunction);
void _CUSBCH340Device(TUSBCH340Device *pThis);

boolean USBCH340DeviceConfigure(TUSBFunction *pUSBFunction);

#endif
