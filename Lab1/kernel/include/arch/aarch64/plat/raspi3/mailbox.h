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

#ifndef ARCH_AARCH64_PLAT_RASPI3_MAILBOX_H
#define ARCH_AARCH64_PLAT_RASPI3_MAILBOX_H

#include <common/vars.h>

#define MMIO_BASE      (KBASE + 0x3F000000)
#define VIDEOCORE_MBOX (MMIO_BASE + 0x0000B880)
#define MBOX_READ      ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x0))
#define MBOX_POLL      ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x10))
#define MBOX_SENDER    ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x14))
#define MBOX_STATUS    ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x18))
#define MBOX_CONFIG    ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x1C))
#define MBOX_WRITE     ((volatile unsigned int*)(VIDEOCORE_MBOX + 0x20))
#define MBOX_RESPONSE  0x80000000
#define MBOX_FULL      0x80000000
#define MBOX_EMPTY     0x40000000

/* channels */
#define MBOX_CH_POWER 0
#define MBOX_CH_FB    1
#define MBOX_CH_VUART 2
#define MBOX_CH_VCHIQ 3
#define MBOX_CH_LEDS  4
#define MBOX_CH_BTNS  5
#define MBOX_CH_TOUCH 6
#define MBOX_CH_COUNT 7
#define MBOX_CH_PROP  8

/* tags */
#define MBOX_TAG_GetBoardRevision 0x00010002
#define MBOX_TAG_GetVCMemory      0x00010006
#define MBOX_TAG_GETSERIAL        0x10004
#define MBOX_TAG_LAST             0
#define MBOX_TAG_POWER_STATE      0x00028001
#define MBOX_TAG_REQUEST          MBOX_TAG_POWER_STATE << 8

#endif /* ARCH_AARCH64_PLAT_RASPI3_MAILBOX_H */