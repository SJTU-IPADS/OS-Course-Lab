//
// devicenameservice.c
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#include <uspi/devicenameservice.h>
#include <uspi/util.h>
#include <uspi/assert.h>
#include <uspios.h>
#include <stdio.h>

static TDeviceNameService *s_pThis = 0;

void DeviceNameService(TDeviceNameService *pThis)
{
	assert(pThis != 0);

	pThis->m_pList = 0;

	assert(s_pThis == 0);
	s_pThis = pThis;
	pThis->next_id = 1;
}

void _DeviceNameService(TDeviceNameService *pThis)
{
	assert(pThis != 0);

	while (pThis->m_pList != 0) {
		TDeviceInfo *pNext = pThis->m_pList->pNext;

		assert(pThis->m_pList->pName != 0);
		free(pThis->m_pList->pName);
		pThis->m_pList->pName = 0;

		pThis->m_pList->pDevice = 0;

		free(pThis->m_pList);

		pThis->m_pList = pNext;
	}

	s_pThis = 0;
}

void DeviceNameServiceAddDevice(TDeviceNameService *pThis, const char *pName,
				const char *pVendor, void *pDevice,
				boolean bBlockDevice, int dev_class)
{
	assert(pThis != 0);

	TDeviceInfo *pInfo = (TDeviceInfo *)malloc(sizeof(TDeviceInfo));
	assert(pInfo != 0);

	assert(pName != 0);
	pInfo->pName = (char *)malloc(strlen(pName) + 1);
	pInfo->pVendor = (char *)malloc(strlen(pVendor) + 1);
	assert(pInfo->pName != 0);
	strcpy(pInfo->pName, pName);
	strcpy(pInfo->pVendor, pVendor);

	assert(pDevice != 0);
	pInfo->pDevice = pDevice;

	pInfo->bBlockDevice = bBlockDevice;
	pInfo->dev_id = __sync_fetch_and_add(&pThis->next_id, 1);
	pInfo->dev_class = dev_class;

	pthread_mutex_init(&pInfo->lock, 0);
	pInfo->pNext = pThis->m_pList;
	pThis->m_pList = pInfo;
}

void *DeviceNameServiceGetDevice(TDeviceNameService *pThis, const char *pName,
				 boolean bBlockDevice)
{
	assert(pThis != 0);
	assert(pName != 0);

	TDeviceInfo *pInfo = pThis->m_pList;
	while (pInfo != 0) {
		assert(pInfo->pName != 0);
		if (strcmp(pName, pInfo->pName) == 0
		    /* && pInfo->bBlockDevice == bBlockDevice */) {
			assert(pInfo->pDevice != 0);
			return pInfo->pDevice;
		}

		pInfo = pInfo->pNext;
	}

	return 0;
}

void ShowDeviceNames(TDeviceNameService *pThis)
{
	LogWrite("[tmac]", LOG_NOTICE, "ShowDeviceNames!");

	if (pThis == 0) {
		LogWrite("[tmac]", LOG_NOTICE, "empty.");
		return;
	}

	TDeviceInfo *pInfo = pThis->m_pList;
	if (pInfo == 0) {
		LogWrite("[tmac]", LOG_NOTICE, "empty.");
		return;
	}

	while (pInfo != 0) {
		assert(pInfo->pName != 0);
		LogWrite("[tmac]", LOG_NOTICE, "pInfo->pName %s", pInfo->pName);
		pInfo = pInfo->pNext;
	}
}

TDeviceNameService *DeviceNameServiceGet(void)
{
	// assert (s_pThis != 0);

	if (s_pThis == 0) {
		LogWrite("[tmac]", LOG_NOTICE, "Service empty now.");
		return 0;
	}

	return s_pThis;
}

TDeviceInfo *DeviceNameServiceGetInfo(TDeviceNameService *pThis,
				      const char *devname)
{
	assert(pThis != 0);
	assert(devname != 0);

	TDeviceInfo *pInfo = pThis->m_pList;
	while (pInfo != 0) {
		assert(pInfo->pName != 0);
		if (strcmp(devname, pInfo->pName) == 0) {
			assert(pInfo->pDevice != 0);
			return pInfo;
		}

		pInfo = pInfo->pNext;
	}

	return NULL;
}

TDeviceInfo *DeviceNameServiceGetInfoByID(TDeviceNameService *pThis, int dev_id)
{
	assert(pThis != 0);

	TDeviceInfo *pInfo = pThis->m_pList;
	while (pInfo != 0) {
		assert(pInfo->pName != 0);
		if (pInfo->dev_id == dev_id) {
			assert(pInfo->pDevice != 0);
			return pInfo;
		}

		pInfo = pInfo->pNext;
	}

	return NULL;
}

void DeviceNameServiceListAllDevice(TDeviceNameService *pThis)
{
	if (pThis == 0) {
		printf("Empty DeviceNameService!\n");
		return;
	}

	TDeviceInfo *pInfo = pThis->m_pList;
	while (pInfo) {
		printf("USBDev: %s, dev id:%d, dev class:%d\n",
		       pInfo->pName,
		       pInfo->dev_id,
		       pInfo->dev_class);
		pInfo = pInfo->pNext;
	}
}
