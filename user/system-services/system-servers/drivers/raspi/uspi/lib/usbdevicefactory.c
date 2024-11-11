//
// usbdevicefactory.c
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright (C) 2014-2018  R. Stange <rsta2@o2online.de>
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
#include <uspi/usbdevicefactory.h>
#include <uspios.h>
#include <uspi/assert.h>

// for factory
#include <uspi/usbstandardhub.h>
#include <uspi/usbmassdevice.h>
#include <uspi/usbkeyboard.h>
#include <uspi/usbmouse.h>
#include <uspi/usbgamepad.h>
#include <uspi/usbmidi.h>
#include <uspi/smsc951x.h>
#include <uspi/lan7800.h>
#include <uspi/usbch340.h>
#include <uspi/usbuvc.h>
#include <uspi/usbcp210x.h>

TUSBFunction *USBDeviceFactoryGetDevice(TUSBFunction *pParent, TString *pName)
{
	assert(pParent != 0);
	assert(pName != 0);

	TUSBFunction *pResult = 0;

	if (StringCompare(pName, "int9-0-2") == 0
	    || StringCompare(pName, "int9-0-0") == 0) {
		TUSBStandardHub *pDevice =
			(TUSBStandardHub *)malloc(sizeof(TUSBStandardHub));
		assert(pDevice != 0);
		USBStandardHub(pDevice, pParent);
		pResult = (TUSBFunction *)pDevice;
	} else if (StringCompare(pName, "int8-6-50") == 0) {
		TUSBBulkOnlyMassStorageDevice *pDevice =
			(TUSBBulkOnlyMassStorageDevice *)malloc(
				sizeof(TUSBBulkOnlyMassStorageDevice));
		assert(pDevice != 0);
		USBBulkOnlyMassStorageDevice(pDevice, pParent);
		pResult = (TUSBFunction *)pDevice;
	} else if (StringCompare(pName, "int3-1-1") == 0) {
		TUSBKeyboardDevice *pDevice = (TUSBKeyboardDevice *)malloc(
			sizeof(TUSBKeyboardDevice));
		assert(pDevice != 0);
		USBKeyboardDevice(pDevice, pParent);
		pResult = (TUSBFunction *)pDevice;
	} else if (StringCompare(pName, "int3-1-2") == 0) {
		TUSBMouseDevice *pDevice =
			(TUSBMouseDevice *)malloc(sizeof(TUSBMouseDevice));
		assert(pDevice != 0);
		USBMouseDevice(pDevice, pParent);
		pResult = (TUSBFunction *)pDevice;
	} else if (StringCompare(pName, "ven424-ec00") == 0) {
		TSMSC951xDevice *pDevice =
			(TSMSC951xDevice *)malloc(sizeof(TSMSC951xDevice));
		assert(pDevice != 0);
		SMSC951xDevice(pDevice, pParent);
		pResult = (TUSBFunction *)pDevice;
	} else if (StringCompare(pName, "ven424-7800") == 0) {
		TLAN7800Device *pDevice =
			(TLAN7800Device *)malloc(sizeof(TLAN7800Device));
		assert(pDevice != 0);
		LAN7800Device(pDevice, pParent);
		pResult = (TUSBFunction *)pDevice;
	} else if (StringCompare(pName, "int3-0-0") == 0) {
		TUSBGamePadDevice *pDevice =
			(TUSBGamePadDevice *)malloc(sizeof(TUSBGamePadDevice));
		assert(pDevice != 0);
		USBGamePadDevice(pDevice, pParent);
		pResult = (TUSBFunction *)pDevice;
	} else if (StringCompare(pName, "int1-3-0") == 0) {
		TUSBMIDIDevice *pDevice =
			(TUSBMIDIDevice *)malloc(sizeof(TUSBMIDIDevice));
		assert(pDevice != 0);
		USBMIDIDevice(pDevice, pParent);
		pResult = (TUSBFunction *)pDevice;
	}
	// new devices follow
	// tmac: add ch340 here. vendor ID: 1a86, device ID: 7523
	// TODO:
	else if (StringCompare(pName, "ven1a86-7523") == 0) {
		TUSBCH340Device *pDevice =
			(TUSBCH340Device *)malloc(sizeof(TUSBCH340Device));
		assert(pDevice != 0);
		USBCH340Device(pDevice, pParent);
		pResult = (TUSBFunction *)pDevice;
	}
	// zixuan: add suppport for uvc device
	else if (StringCompare(pName, "venbda-5825") == 0) {
		//LogWrite("[Zixuan]", LOG_NOTICE, "venbda-5825 found");
		TUSBUVCDevice *pDevice =
			(TUSBUVCDevice *)malloc(sizeof(TUSBUVCDevice));
		assert(pDevice != 0);
		USBUVCDevice(pDevice, pParent);
		pResult = (TUSBFunction *)pDevice;
	} else if (StringCompare(pName, "ven2bc5-502") == 0) {
		//LogWrite("[Zixuan]", LOG_NOTICE, "ven2bc5-502");
		TUSBUVCDevice *pDevice =
			(TUSBUVCDevice *)malloc(sizeof(TUSBUVCDevice));
		assert(pDevice != 0);
		USBUVCDevice(pDevice, pParent);
		pResult = (TUSBFunction *)pDevice;
	}
	// zixuan: add support for Lidar, idVendor:10c4, idProduct: ea60
	else if (StringCompare(pName, "ven10c4-ea60") == 0) {
		TUSBCp210xDevice *pDevice =
			(TUSBCp210xDevice *)malloc(sizeof(TUSBCp210xDevice));
		assert(pDevice != 0);
		USBCp210xDevice(pDevice, pParent);
		pResult = (TUSBFunction *)pDevice;
	}
	if (pResult != 0) {
		LogWrite("usbdev",
			 LOG_NOTICE,
			 "Using device/interface %s",
			 StringGet(pName));
	}

	_String(pName);
	free(pName);

	return pResult;
}
