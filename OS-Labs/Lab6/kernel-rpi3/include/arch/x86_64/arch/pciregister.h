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

/* 
 * Copyright (c) 2006-2017 Frans Kaashoek, Robert Morris, Russ Cox, Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * ( a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

unsigned int create_mask(int start, int offset)
{
        return ((1 << offset) - 1) << start;
}

#define PCI_CAP_REG 0x34
// The bottom two bits are reserved and have to be masked off
#define PCI_CAP_MASK 0xfc

#define PCI_CAP_TYPE         0
#define PCI_CAP_NEXT         1
#define PCI_CAP_CFG_TYPE     3
#define PCI_CAP_BAR          4
#define PCI_CAP_OFF          8 // 05 byte is padding
#define PCI_CAP_POINTER(reg) (reg & create_mask(0, 8))

// The vendor ID is the last 16 bits of the ID register
#define PCI_VENDOR_ID(value) (value & create_mask(0, 16))

// The device ID if the high 16 bits of the ID register
#define PCI_DEVICE_ID(value) ((value & create_mask(16, 16)) >> 16)
/*
 * PCI config space BAR offset
 */
#define PCI_CFG_BAR_OFF 0x10
#define PCI_CFG_BAR_END 0x28

/**
 * author: Anmol Vatsa <anvatsa@cs.utah.edu>
 *
 * PCI Register definitions and convenience macros to access them
 * http://wiki.osdev.org/PCI#PCI_Device_Structure
 */

/*
 * Device identification register; contains a vendor ID and a device ID.
 */
#define PCI_ID_REG 0x00

#define PCI_VENDOR_SHIFT 0
#define PCI_VENDOR_MASK  0xffff
#define PCI_VENDOR(id)   (((id) >> PCI_VENDOR_SHIFT) & PCI_VENDOR_MASK)

#define PCI_PRODUCT_SHIFT 16
#define PCI_PRODUCT_MASK  0xffff
#define PCI_PRODUCT(id)   (((id) >> PCI_PRODUCT_SHIFT) & PCI_PRODUCT_MASK)

#define PCI_ID_CODE(vid, pid)                          \
        ((((vid)&PCI_VENDOR_MASK) << PCI_VENDOR_SHIFT) \
         | (((pid)&PCI_PRODUCT_MASK) << PCI_PRODUCT_SHIFT))

/*
 * PCI BHLC = BIST/Header Type/Latency Timer/Cache Line Size Register.
 */
#define PCI_BHLC_REG 0x0c

#define PCI_BIST_SHIFT  24
#define PCI_BIST_MASK   0xff
#define PCI_BIST(bhlcr) (((bhlcr) >> PCI_BIST_SHIFT) & PCI_BIST_MASK)

#define PCI_HDRTYPE_SHIFT  16
#define PCI_HDRTYPE_MASK   0xff
#define PCI_HDRTYPE(bhlcr) (((bhlcr) >> PCI_HDRTYPE_SHIFT) & PCI_HDRTYPE_MASK)

#define PCI_HDRTYPE_TYPE(bhlcr) (PCI_HDRTYPE(bhlcr) & 0x7f)

#define IS_PCI_HDRTYPE_PPB(bhlcr) (PCI_HDRTYPE_TYPE(bhlcr) == PCI_HDRTYPE_PPB)

#define PCI_HDRTYPE_MULTIFN(bhlcr) ((PCI_HDRTYPE(bhlcr) & 0x80) != 0)

#define PCI_LATTIMER_SHIFT 8
#define PCI_LATTIMER_MASK  0xff
#define PCI_LATTIMER(bhlcr) \
        (((bhlcr) >> PCI_LATTIMER_SHIFT) & PCI_LATTIMER_MASK)

#define PCI_CACHELINE_SHIFT 0
#define PCI_CACHELINE_MASK  0xff
#define PCI_CACHELINE(bhlcr) \
        (((bhlcr) >> PCI_CACHELINE_SHIFT) & PCI_CACHELINE_MASK)

/*
 * Interrupt Configuration Register; contains interrupt pin and line.
 */
#define PCI_INTERRUPT_REG 0x3c

#define PCI_INTERRUPT_PIN_SHIFT 8
#define PCI_INTERRUPT_PIN_MASK  0xff
#define PCI_INTERRUPT_PIN(icr) \
        (((icr) >> PCI_INTERRUPT_PIN_SHIFT) & PCI_INTERRUPT_PIN_MASK)

#define PCI_INTERRUPT_LINE_SHIFT 0
#define PCI_INTERRUPT_LINE_MASK  0xff
#define PCI_INTERRUPT_LINE(icr) \
        (((icr) >> PCI_INTERRUPT_LINE_SHIFT) & PCI_INTERRUPT_LINE_MASK)

/*
 * PCI Class and Revision Register; defines type and revision of device.
 */
#define PCI_CLASS_REG 0x08

#define PCI_CLASS_SHIFT 24
#define PCI_CLASS_MASK  0xff
#define PCI_CLASS(cr)   (((cr) >> PCI_CLASS_SHIFT) & PCI_CLASS_MASK)

#define PCI_SUBCLASS_SHIFT 16
#define PCI_SUBCLASS_MASK  0xff
#define PCI_SUBCLASS(cr)   (((cr) >> PCI_SUBCLASS_SHIFT) & PCI_SUBCLASS_MASK)

#define PCI_DEVICE_CLASS_NETWORK_CONTROLLER 2

/*
 * Command and status register.
 */
#define PCI_COMMAND_STATUS_REG 0x04

#define PCI_COMMAND_IO_ENABLE        0x00000001
#define PCI_COMMAND_MEM_ENABLE       0x00000002
#define PCI_COMMAND_MASTER_ENABLE    0x00000004
#define PCI_COMMAND_CAPABALITES_LIST 0x00000010

/*
 * Mapping registers
 */
#define PCI_MAPREG_START 0x10
#define PCI_MAPREG_END   0x28

#define PCI_MAPREG_NUM(offset) (((unsigned)(offset)-PCI_MAPREG_START) / 4)

#define PCI_MAPREG_TYPE_MASK 0x00000001
#define PCI_MAPREG_TYPE(mr)  ((mr)&PCI_MAPREG_TYPE_MASK)

#define PCI_MAPREG_TYPE_MEM 0x00000000
#define PCI_MAPREG_TYPE_IO  0x00000001

#define PCI_MAPREG_MEM_TYPE_MASK 0x00000006
#define PCI_MAPREG_MEM_TYPE(mr)  ((mr)&PCI_MAPREG_MEM_TYPE_MASK)

#define PCI_MAPREG_MEM_TYPE_32BIT 0x00000000
#define PCI_MAPREG_MEM_TYPE_64BIT 0x00000004

#define PCI_MAPREG_MEM_ADDR_MASK 0xfffffff0
#define PCI_MAPREG_MEM_ADDR(mr)  ((mr)&PCI_MAPREG_MEM_ADDR_MASK)
/*
 * For 64-bit Memory Space BARs, you calculate
 * ((BAR[x] & 0xFFFFFFF0) + ((BAR[x + 1] & 0xFFFFFFFF) << 32))
 */
#define PCI_MAPREG_MEM_ADDR_64BIT(mr1, mr2) \
        (((u64)PCI_MAPREG_MEM_ADDR(mr1)) + (((u64)((mr2) & 0xffffffff)) << 32))

#define PCI_MAPREG_MEM_SIZE(mr) \
        (PCI_MAPREG_MEM_ADDR(mr) & -PCI_MAPREG_MEM_ADDR(mr))

#define PCI_MAPREG_IO_ADDR_MASK 0xfffffffc
#define PCI_MAPREG_IO_ADDR(mr)  ((mr)&PCI_MAPREG_IO_ADDR_MASK)

#define PCI_MAPREG_IO_SIZE(mr) \
        (PCI_MAPREG_IO_ADDR(mr) & -PCI_MAPREG_IO_ADDR(mr))
