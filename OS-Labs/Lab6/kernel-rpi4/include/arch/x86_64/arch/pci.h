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

#include <common/types.h>
#include <arch/sync.h>

/*
 * PCI software configuration space access mechanism 1
 *
 * https://wiki.osdev.org/PCI#Configuration_Space_Access_Mechanism_.231
 */
#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

// Forward declaration
struct pci_device;

#define uint64   u64
#define uint32   u32
#define uint16   u16
#define uint8    u8
#define uint64_t u64
#define uint32_t u32
#define uint16_t u16
#define uint8_t  u8

/*
 * The PCI specification specifies 8 bits for the bus identifier, 5 bits for
 * the device and 3 bits for selecting a particular function. This is the BDF
 * (Bus Device Function) address of a PCI device
 */
#define PCI_MAX_DEVICES 32

/*
 * Defines a PCI bus. Only considering it's parent bridge and the bus number
 * for now
 */
struct pci_bus {
        // A bridge is a type of PCI device which just forwards all transactions
        // if they are destined for any device behind it. The PCI host bridge is
        // the common interface for all other PCI devices/buses on a system
        struct pci_device *parent_bridge;
        u32 bus_num;
};

struct pci_device {
        enum { PCI_FREE, PCI_USED } state;
        // The bus this device is on
        struct pci_bus *bus;

        // The device number of the device on the bus
        u32 dev;
        // The function represented by this struct
        u32 func;

        u32 dev_id;
        u32 dev_class;

        u64 reg_base[6];
        u32 reg_size[6];
        // Virtio spec v1.0 only defines 5 types of capabilites.
        u8 cap[6]; // Maps cap type to offset within the pci config space.
        u8 cap_bar[6]; // Maps cap type to their BAR number
        u32 cap_off[6]; // Map cap type to offset within bar

        u8 irq_line;
        u8 irq_pin;
        u64 membase;
        u32 iobase;

        /* Only for virtio net device */
        u8 mac[8];
};

#define NPCI 50

// The PCI device ID of is VIRTIO_DEVICE_ID_BASE + virtio_device
#define VIRTIO_DEVICE_ID_BASE 0x1040
// A virtio device will always have this vendor id
#define VIRTIO_VENDOR_ID 0x1AF4

#define VIRTIO_HOST_F_OFF  0x0000
#define VIRTIO_GUEST_F_OFF 0x0004
#define VIRTIO_QADDR_OFF   0x0008

#define VIRTIO_QSIZE_OFF   0x000C
#define VIRTIO_QSEL_OFF    0x000E
#define VIRTIO_QNOTFIY_OFF 0x0010

#define VIRTIO_DEV_STATUS_OFF   0x0012
#define VIRTIO_ISR_STATUS_OFF   0x0013
#define VIRTIO_DEV_SPECIFIC_OFF 0x0014
/* if msi is enabled, device specific headers shift by 4 */
#define VIRTIO_MSI_ADD_OFF   0x0004
#define VIRTIO_STATUS_DRV    0x02
#define VIRTIO_STATUS_ACK    0x01
#define VIRTIO_STATUS_DRV_OK 0x04
#define VIRTIO_STATUS_FAIL   0x80

// Virtio device IDs
enum VIRTIO_DEVICE {
        RESERVED = 0,
        NETWORK_CARD = 1,
        BLOCK_DEVICE = 2,
        CONSOLE = 3,
        ENTROPY_SOURCE = 4,
        MEMORY_BALLOONING_TRADITIONAL = 5,
        IO_MEMORY = 6,
        RPMSG = 7,
        SCSI_HOST = 8,
        NINEP_TRANSPORT = 9,
        MAC_80211_WLAN = 10,
        RPROC_SERIAL = 11,
        VIRTIO_CAIF = 12,
        MEMORY_BALLOON = 13,
        GPU_DEVICE = 14,
        CLOCK_DEVICE = 15,
        INPUT_DEVICE = 16
};

// Transitional vitio device ids
enum TRANSITIONAL_VIRTIO_DEVICE {
        T_NETWORK_CARD = 0X1000,
        T_BLOCK_DEVICE = 0X1001,
        T_MEMORY_BALLOONING_TRADITIONAL = 0X1002,
        T_CONSOLE = 0X1003,
        T_SCSI_HOST = 0X1004,
        T_ENTROPY_SOURCE = 0X1005,
        T_NINEP_TRANSPORT = 0X1006
};

struct virtio_pci_cap {
        // Generic PCI field: PCI_CAP_ID_VNDR, 0
        uint8 cap_vendor;
        // Generic PCI field: next ptr, 1
        uint8 cap_next;
        // Generic PCI field: capability length, 2
        uint8 cap_len;
/* Common configuration */
#define VIRTIO_PCI_CAP_COMMON_CFG 1
/* Notifications */
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
/* ISR Status */
#define VIRTIO_PCI_CAP_ISR_CFG 3
/* Device specific configuration */
#define VIRTIO_PCI_CAP_DEVICE_CFG 4
/* PCI configuration access */
#define VIRTIO_PCI_CAP_PCI_CFG 5
        // Identifies the structure, 3
        uint8 cfg_type;
        // Where to find it, 4
        uint8 bar;
        // Pad to full dword, 5
        uint8 padding[3];
        // Offset within bar, 8
        uint32 offset;
        // Length of the structure, in bytes, 12
        uint32 length;
};

extern struct pci_device pcidevs[NPCI];
extern int pcikeys[NPCI];

static inline u8 inb(u16 port)
{
        u8 data;

        asm volatile("in %1,%0" : "=a"(data) : "d"(port));
        return data;
}

static inline u16 inw(int port)
{
        u16 data;
        asm volatile("inw %w1,%0" : "=a"(data) : "d"(port));
        return data;
}

static inline u32 inl(int port)
{
        u32 data;
        asm volatile("inl %w1,%0" : "=a"(data) : "d"(port));
        return data;
}

static inline void outl(int port, u32 data)
{
        asm volatile("outl %0,%w1" : : "a"(data), "d"(port));
}

static inline void outb(u16 port, u8 data)
{
        asm volatile("out %0,%1" : : "a"(data), "d"(port));
}

static inline void outw(u16 port, u16 data)
{
        asm volatile("out %0,%1" : : "a"(data), "d"(port));
}

/*
 * Macro to create the 32 bit register values for a PCI transaction. `off` here
 * is the register value in the PCI config space of the device specified by
 * `dev` on the bus specificed by `bus` and the function specified by `fn`.
 *
 * The MSB of the 32 bit value needs to be 1 in order to mark it as
 * a configuration transaction
 */
#define PCI_FORMAT(bus, dev, fn, off) \
        ({ 0x80000000 | bus << 16 | dev << 11 | fn << 8 | off; })

#define PCI_CONF_READ8(bus, dev, func, reg)                      \
        (outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
         inb(PCI_CONFIG_DATA + ((reg)&3)))
#define PCI_CONF_READ16(bus, dev, func, reg)                     \
        (outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
         inw(PCI_CONFIG_DATA + ((reg)&2)))
#define PCI_CONF_READ32(bus, dev, func, reg)                     \
        (outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
         inl(PCI_CONFIG_DATA))

#define PCI_CONF_WRITE8(bus, dev, func, reg, val)                \
        (outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
         outb(PCI_CONFIG_DATA + ((reg)&3), (val)))
#define PCI_CONF_WRITE16(bus, dev, func, reg, val)               \
        (outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
         outw(PCI_CONFIG_DATA + ((reg)&2), (val)))
#define PCI_CONF_WRITE32(bus, dev, func, reg, val)               \
        (outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
         outl(PCI_CONFIG_DATA, (val)))

/*
 * Class codes of PCI devices at their offsets
 */
extern char *PCI_CLASSES[18];

/*
 * Reads the register at offset `off` in the PCI config space of the device
 * `dev`
 */
static inline uint32 confread32(struct pci_device *dev, uint32 off)
{
        return PCI_CONF_READ32(dev->bus->bus_num, dev->dev, dev->func, off);
}

static inline uint32 confread16(struct pci_device *dev, uint32 off)
{
        return PCI_CONF_READ16(dev->bus->bus_num, dev->dev, dev->func, off);
}

static inline uint32 confread8(struct pci_device *dev, uint32 off)
{
        return PCI_CONF_READ8(dev->bus->bus_num, dev->dev, dev->func, off);
}

/*
 * Writes the provided value `value` at the register `off` for the device `dev`
 */
static inline void conf_write32(struct pci_device *dev, uint32 off,
                                uint32 value)
{
        return PCI_CONF_WRITE32(
                dev->bus->bus_num, dev->dev, dev->func, off, value);
}

/*
 * Writes the provided value `value` at the register `off` for the device `dev`
 */
static inline void conf_write16(struct pci_device *dev, uint32 off,
                                uint16 value)
{
        return PCI_CONF_WRITE16(
                dev->bus->bus_num, dev->dev, dev->func, off, value);
}

/*
 * Writes the provided value `value` at the register `off` for the device `dev`
 */
static inline void conf_write8(struct pci_device *dev, uint32 off, uint8 value)
{
        return PCI_CONF_WRITE8(
                dev->bus->bus_num, dev->dev, dev->func, off, value);
}

static inline u8 ioread8(u8 *addr)
{
        u8 ret;
        mb();
        ret = *addr;
        rmb();
        return ret;
}

static inline void iowrite8(u8 value, u8 *addr)
{
        mb();
        *addr = value;
        wmb();
}

static inline u16 ioread16(u16 *addr)
{
        u16 ret;
        mb();
        ret = *addr;
        rmb();
        return ret;
}

static inline void iowrite16(u16 value, u16 *addr)
{
        mb();
        *addr = value;
        wmb();
}

static inline u32 ioread32(u32 *addr)
{
        u32 ret;
        mb();
        ret = *addr;
        rmb();
        return ret;
}

static inline void iowrite32(u32 value, u32 *addr)
{
        mb();
        *addr = value;
        wmb();
}

int pci_init(void);
void arch_get_pci_device(int class, u64 pci_dev_uaddr);
