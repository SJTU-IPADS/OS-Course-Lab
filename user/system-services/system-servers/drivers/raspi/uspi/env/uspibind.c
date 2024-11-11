//
// uspibind.cpp
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
#include <uspios.h>
#include <uspienv/timer.h>
#include <uspienv/interrupt.h>
#include <uspienv/bcmpropertytags.h>
#include <uspienv/logger.h>
#include <uspienv/debug.h>
#include <uspienv/util.h>
#include <uspienv/assert.h>

#include <stdio.h>

#define kinfo printf

#define smp_mb() asm volatile("dmb ish\n\t")

#define write32 put32
#define read32  get32

void TimerSimpleMsDelay(unsigned ms)
{
	MsDelay(ms);
}

int SetPowerStateOn(unsigned nDeviceId)
{
	//kinfo("[tmac] func: %s, line: %d\n", __func__, __LINE__);

	TBcmPropertyTags Tags;
	BcmPropertyTags(&Tags);
	TPropertyTagPowerState PowerState;
	PowerState.nDeviceId = nDeviceId;
	PowerState.nState = POWER_STATE_ON | POWER_STATE_WAIT;

	//kinfo("[tmac] func: %s, line: %d\n", __func__, __LINE__);

	int ret;
	ret = BcmPropertyTagsGetTag(&Tags,
				    PROPTAG_SET_POWER_STATE,
				    &PowerState,
				    sizeof PowerState,
				    8);
	//kinfo("BcmPropertyTagsGetTag returns %d\n", ret);

	if (!ret || (PowerState.nState & POWER_STATE_NO_DEVICE)
	    || !(PowerState.nState & POWER_STATE_ON)) {
		_BcmPropertyTags(&Tags);

		//kinfo("[tmac] func: %s, line: %d\n", __func__, __LINE__);
		return 0;
	}

	_BcmPropertyTags(&Tags);

	return 1;
}

#include <uspienv/bcmpropertytags.h>
#include <uspienv/util.h>
#include <uspienv/synchronize.h>
#include <uspienv/bcm2835.h>
#include <uspienv/sysconfig.h>
#include <uspienv/assert.h>

typedef struct TPropertyBuffer {
	u32 nBufferSize; // bytes
	u32 nCode;
#define CODE_REQUEST          0x00000000
#define CODE_RESPONSE_SUCCESS 0x80000000
#define CODE_RESPONSE_FAILURE 0x80000001
	u8 Tags[0];
	// end tag follows
} TPropertyBuffer;

void BcmPropertyTags(TBcmPropertyTags *pThis)
{
	assert(pThis != 0);

	BcmMailBox(&pThis->m_MailBox, BCM_MAILBOX_PROP_OUT);
}

void _BcmPropertyTags(TBcmPropertyTags *pThis)
{
	assert(pThis != 0);

	_BcmMailBox(&pThis->m_MailBox);
}

char reserved_buffer[0x100000] __attribute__((section(".test_reserverd")))
__attribute__((aligned(0x1000)));
//char reserved_buffer[0x100000] __attribute__((aligned(0x1000)));

#include <chcore/syscall.h>

/*
 * For accessing MailBox, the used address should be lower than 4G.
 * BUS_ADDRESS.
 */

static int get_pmo_paddr(cap_t pmo, unsigned long pmo_size, unsigned long *buffer_paddr)
{
        unsigned long vaddr;
        int ret;

        vaddr = (unsigned long)chcore_auto_map_pmo(pmo, pmo_size, VM_READ);
        ret = usys_get_phys_addr((void *)vaddr, (void *)buffer_paddr);
        chcore_auto_unmap_pmo(pmo, vaddr, pmo_size);
        return ret;
}

boolean BcmPropertyTagsGetTag(TBcmPropertyTags *pThis, u32 nTagId, void *pTag,
			      unsigned nTagSize, unsigned nRequestParmSize)
{
	assert(pThis != 0);

	assert(pTag != 0);
	assert(nTagSize >= sizeof(TPropertyTagSimple));
	unsigned nBufferSize = sizeof(TPropertyBuffer) + nTagSize + sizeof(u32);
	assert((nBufferSize & 3) == 0);

	//kinfo("[tmac] func: %s, line: %d\n", __func__, __LINE__);

/* An arbitrary address (non-zero). */
#define ADDR 0x4000
#define SIZE 0x1000

	boolean res = FALSE;
	int ret;
	cap_t buffer_pmo;
	u64 buffer_paddr;

	/* Allocate physical memory and get its physical address. */
	buffer_pmo = usys_create_pmo(SIZE, PMO_DATA_NOCACHE);

        ret = get_pmo_paddr(buffer_pmo, SIZE, &buffer_paddr);
	if (ret < 0)
		goto out;

	/* Use the allocated physical memory as a device pmo and maps it. */
	ret = usys_map_pmo(0, buffer_pmo, ADDR, VM_READ | VM_WRITE);
	if (ret < 0)
		goto out_free_pmo;

	TPropertyBuffer *pBuffer = (TPropertyBuffer *)ADDR;

	//kinfo("[tmac] func: %s, line: %d\n", __func__, __LINE__);

	pBuffer->nBufferSize = nBufferSize;

	//kinfo("[tmac] func: %s, line: %d\n", __func__, __LINE__);
	pBuffer->nCode = CODE_REQUEST;

	//kinfo("[tmac] func: %s, line: %d\n", __func__, __LINE__);
	unsigned i;
	for (i = 0; i < nTagSize; ++i)
		pBuffer->Tags[i] = ((char *)pTag)[i];
	// memcpy (pBuffer->Tags, pTag, nTagSize);

	//kinfo("[tmac] func: %s, line: %d\n", __func__, __LINE__);

	TPropertyTag *pHeader = (TPropertyTag *)pBuffer->Tags;
	pHeader->nTagId = nTagId;
	pHeader->nValueBufSize = nTagSize - sizeof(TPropertyTag);
	pHeader->nValueLength = nRequestParmSize & ~VALUE_LENGTH_RESPONSE;

	u32 *pEndTag = (u32 *)(pBuffer->Tags + nTagSize);
	*pEndTag = PROPTAG_END;

	//kinfo("[tmac] func: %s, line: %d\n", __func__, __LINE__);
	// ret = usys_get_phys_addr(pBuffer, &pa);
	// assert(ret == 0);
	// assert(pa < 1024 * 1024 * 1024);

	// u32 nBufferAddress = BUS_ADDRESS ((u32)(u64)pBuffer);
	u32 nBufferAddress = BUS_ADDRESS((u32)buffer_paddr);
	if (BcmMailBoxWriteRead(&pThis->m_MailBox, nBufferAddress)
	    != nBufferAddress)
		goto out_unmap;

	//kinfo("[tmac] func: %s, line: %d\n", __func__, __LINE__);

	smp_mb();
	// DataMemBarrier ();

	if (pBuffer->nCode != CODE_RESPONSE_SUCCESS)
		goto out_unmap;

	//kinfo("[tmac] func: %s, line: %d\n", __func__, __LINE__);
	if (!(pHeader->nValueLength & VALUE_LENGTH_RESPONSE))
		goto out_unmap;

	//kinfo("[tmac] func: %s, line: %d\n", __func__, __LINE__);
	pHeader->nValueLength &= ~VALUE_LENGTH_RESPONSE;
	if (pHeader->nValueLength == 0)
		goto out_unmap;

	for (i = 0; i < nTagSize; ++i)
		((char *)pTag)[i] = pBuffer->Tags[i];

	//memcpy (pTag, pBuffer->Tags, nTagSize);

	res = TRUE;

out_unmap:
	usys_unmap_pmo(0, buffer_pmo, ADDR);
out_free_pmo:
        usys_revoke_cap(buffer_pmo, false);
out:
        usys_revoke_cap(buffer_pmo, false);
	return res;
}

#include <uspienv/bcmmailbox.h>
#include <uspienv/memio.h>
#include <uspienv/synchronize.h>
#include <uspienv/timer.h>
#include <uspienv/assert.h>

void BcmMailBoxFlush(TBcmMailBox *pThis);
unsigned BcmMailBoxRead(TBcmMailBox *pThis);
void BcmMailBoxWrite(TBcmMailBox *pThis, unsigned nData);

void BcmMailBox(TBcmMailBox *pThis, unsigned nChannel)
{
	assert(pThis != 0);

	pThis->m_nChannel = nChannel;
}

void _BcmMailBox(TBcmMailBox *pThis)
{
	assert(pThis != 0);
}

unsigned BcmMailBoxWriteRead(TBcmMailBox *pThis, unsigned nData)
{
	assert(pThis != 0);

	smp_mb();
	// DataMemBarrier ();

	BcmMailBoxFlush(pThis);

	BcmMailBoxWrite(pThis, nData);

	unsigned nResult = BcmMailBoxRead(pThis);

	smp_mb();
	// DataMemBarrier ();

	return nResult;
}

void BcmMailBoxFlush(TBcmMailBox *pThis)
{
	assert(pThis != 0);

	//kinfo("%s starts\n", __func__);

	while (!(read32(MAILBOX0_STATUS) & MAILBOX_STATUS_EMPTY)) {
		read32(MAILBOX0_READ);

		TimerSimpleMsDelay(20);
	}
	//kinfo("%s done\n", __func__);
}

unsigned BcmMailBoxRead(TBcmMailBox *pThis)
{
	assert(pThis != 0);

	unsigned nResult;

	//kinfo("%s starts\n", __func__);

	do {
		while (read32(MAILBOX0_STATUS) & MAILBOX_STATUS_EMPTY) {
			// do nothing
		}
		nResult = read32(MAILBOX0_READ);
	} while ((nResult & 0xF)
		 != pThis->m_nChannel); // channel number is in the lower 4 bits

	//kinfo("%s done\n", __func__);
	return nResult & ~0xF;
}

void BcmMailBoxWrite(TBcmMailBox *pThis, unsigned nData)
{
	assert(pThis != 0);

	//kinfo("%s starts\n", __func__);
	while (read32(MAILBOX1_STATUS) & MAILBOX_STATUS_FULL) {
		// do nothing
	}

	assert((nData & 0xF) == 0);
	write32(MAILBOX1_WRITE,
		pThis->m_nChannel
			| nData); // channel number is in the lower 4 bits
	//kinfo("%s done\n", __func__);
}
