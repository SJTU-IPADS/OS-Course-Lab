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

#pragma once

/*
 * Note that all these APB registers are accessed
 * through virtual addresses, the physical addresses
 * of these registers are 0x8xxxxxxx.
 */

/* Timers */
#define PRESCALER_VALUE_REG	0xf0000300
#define PRESCALER_RELOAD_REG	0xf0000304
#define TIMER_CONFIG_REG	0xf0000308

/* idx from 1 to 4 */
#define TIMERx_VAL_REG(idx)	((idx) * 0x10 + 0xf0000300)
#define TIMERx_RELOAD_REG(idx)	((idx) * 0x10 + 0xf0000304)
#define TIMERx_CTRL_REG(idx)	((idx) * 0x10 + 0xf0000308)

#define TIMER_CONFIG_GET_CNT(x)	((x) & 0x7)
#define TIMER_CONFIG_GET_IRQ(x)	(((x) >> 3) & 0x1f)

/* Latch timers */
#define LPRESCALER_VALUE_REG    0xf0100000
#define LPRESCALER_RELOAD_REG   0xf0100004
#define LTIMER_CONFIG_REG	0xf0100008
#define LTIMER_LATCH_SEL_REG    0xf010000C

/* idx from 1 to 2 */
#define LTIMERx_VAL_REG(idx)     ((idx) * 0x10 + 0xf0100000)
#define LTIMERx_RELOAD_REG(idx)  ((idx) * 0x10 + 0xf0100004)
#define LTIMERx_CTRL_REG(idx)    ((idx) * 0x10 + 0xf0100008)
#define LTIMERx_LATCH_VALUE(idx) ((idx) * 0x10 + 0xf010000C)

#define TIMER_MAX_VAL		0xffffffff
#define T_SYS_CLOCK		5  // 5ns
#define PRESCALER_RELOAD_VAL	199  // Timing length = (TIMER_RELOAD_VAL + 1) * (PRESCALER_RELOAD_VAL + 1) * T_SYS_CLOCK(5ns).

#define TIMER_CTRL_EN	0x1  // Enable timer
#define TIMER_CTRL_RS	0x2  // Reload automatically
#define TIMER_CTRL_LD	0x4  // Load the reload value immediately
#define TIMER_CTRL_IE	0x8  // Enable interrupt
#define TIMER_CTRL_IP	0x10 // Interrupt flag
#define TIMER_CTRL_CH	0x20 // cascade enable
