//
// synchronize.c
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
#include <uspi/synchronize.h>
#include <uspi/types.h>
#include <uspi/assert.h>

#include <chcore/syscall.h>

// #ifndef AARCH64
// #define EnableInterrupts()  __asm volatile("cpsie i")
// #define DisableInterrupts() __asm volatile("cpsid i")
// #else
// #define EnableInterrupts()  __asm volatile("msr DAIFClr, #2")
// #define DisableInterrupts() __asm volatile("msr DAIFSet, #2")
// #endif

// static volatile unsigned s_nCriticalLevel = 0;
// static volatile boolean s_bWereEnabled;

// void uspi_EnterCritical(void)
// {
// #ifndef AARCH64
// 	u32 nFlags;
// 	asm volatile("mrs %0, cpsr" : "=r"(nFlags));
// #else
// 	// u64 nFlags;
// 	// asm volatile ("mrs %0, daif" : "=r" (nFlags));
// #endif

// 	// DisableInterrupts ();

// 	// printf("%s\n", __func__);
// 	// usys_irq_register(10);

// 	if (s_nCriticalLevel++ == 0) {
// 		// s_bWereEnabled = nFlags & 0x80 ? FALSE : TRUE;
// 	}

// 	DataMemBarrier();
// }

// void uspi_LeaveCritical(void)
// {
// 	DataMemBarrier();

// 	// printf("%s\n", __func__);
// 	// usys_irq_register(11);

// 	assert(s_nCriticalLevel > 0);
// 	if (--s_nCriticalLevel == 0) {
// 		// if (s_bWereEnabled)
// 		{
// 			// EnableInterrupts ();
// 			// usys_irq_register(11);
// 		}
// 	}
// }

#ifndef AARCH64

#if RASPPI == 1

//
// Cache maintenance operations for ARMv6
//
// NOTE: The following functions should hold all variables in CPU registers. Currently this will be
//	 ensured using maximum optimation (see circle/synchronize.h).
//
//	 The following numbers can be determined (dynamically) using CTR.
//	 As long we use the ARM1176JZF-S implementation in the BCM2835 these static values will work:
//

#define DATA_CACHE_LINE_LENGTH 32

void uspi_CleanAndInvalidateDataCacheRange(u32 nAddress, u32 nLength)
{
	nLength += DATA_CACHE_LINE_LENGTH;

	while (1) {
		asm volatile("mcr p15, 0, %0, c7, c14,  1"
			     :
			     : "r"(nAddress)
			     : "memory");

		if (nLength < DATA_CACHE_LINE_LENGTH) {
			break;
		}

		nAddress += DATA_CACHE_LINE_LENGTH;
		nLength -= DATA_CACHE_LINE_LENGTH;
	}
}

#else // #if RASPPI == 1

//
// Cache maintenance operations for ARMv7-A
//
// See: ARMv7-A Architecture Reference Manual, Section B4.2.1
//
// NOTE: The following functions should hold all variables in CPU registers. Currently this will be
//	 ensured using the register keyword and maximum optimation (see uspi/synchronize.h).
//
//	 The following numbers can be determined (dynamically) using CTR, CSSELR, CCSIDR and CLIDR.
//	 As long we use the Cortex-A7 implementation in the BCM2836 or the Cortex-A53 implementation
//	 in the BCM2837 these static values will work:
//

#define L1_DATA_CACHE_LINE_LENGTH 64
#define L2_CACHE_LINE_LENGTH      64
#define DATA_CACHE_LINE_LENGTH_MIN \
	64 // min(L1_DATA_CACHE_LINE_LENGTH, L2_CACHE_LINE_LENGTH)

void uspi_CleanAndInvalidateDataCacheRange(u32 nAddress, u32 nLength)
{
	nLength += DATA_CACHE_LINE_LENGTH_MIN;

	while (1) {
		__asm volatile("mcr p15, 0, %0, c7, c14,  1"
			       :
			       : "r"(nAddress)
			       : "memory"); // DCCIMVAC

		if (nLength < DATA_CACHE_LINE_LENGTH_MIN) {
			break;
		}

		nAddress += DATA_CACHE_LINE_LENGTH_MIN;
		nLength -= DATA_CACHE_LINE_LENGTH_MIN;
	}
}

#endif // #if RASPPI == 1

#else // #ifndef AARCH64

//
// Cache maintenance operations for ARMv8-A
//
// NOTE: The following functions should hold all variables in CPU registers. Currently this will be
//	 ensured using the register keyword and maximum optimation (see circle/synchronize.h).
//
//	 The following numbers can be determined (dynamically) using CTR_EL0, CSSELR_EL1, CCSIDR_EL1
//	 and CLIDR_EL1. As long we use the Cortex-A53 implementation in the BCM2837 these static
//	 values will work:
//

#define L1_DATA_CACHE_LINE_LENGTH 64
#define L2_CACHE_LINE_LENGTH      64
#define DATA_CACHE_LINE_LENGTH_MIN \
	64 // min(L1_DATA_CACHE_LINE_LENGTH, L2_CACHE_LINE_LENGTH)

void uspi_CleanAndInvalidateDataCacheRange(u64 nAddress, u64 nLength)
{
	usys_cache_flush(nAddress, nLength, CACHE_CLEAN_AND_INV);
#if 0
	nLength += DATA_CACHE_LINE_LENGTH_MIN;

	while (1)
	{
		asm volatile ("dc civac, %0" : : "r" (nAddress) : "memory");

		if (nLength < DATA_CACHE_LINE_LENGTH_MIN)
		{
			break;
		}

		nAddress += DATA_CACHE_LINE_LENGTH_MIN;
		nLength  -= DATA_CACHE_LINE_LENGTH_MIN;
	}
#endif
}

#endif // #ifndef AARCH64
