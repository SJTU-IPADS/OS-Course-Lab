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

#define TIMER_RESCALER_REG	0xf1000064

/* idx from 0 to 1 */
#define TIMERx_VAL_REG(idx)	((idx) * 0x10 + 0xf1000040)
#define TIMERx_RELOAD_REG(idx)	((idx) * 0x10 + 0xf1000044)
#define TIMERx_CTRL_REG(idx)	((idx) * 0x10 + 0xf1000048)

#define TIMER_CONFIG_GET_CNT(x)	((x) & 0x7)
#define TIMER_CONFIG_GET_IRQ(x)	(((x) >> 3) & 0x1f)

#define TIMER_MAX_VAL		0xffffffff
#define FREQ_MZH		(200 / 3) // in Mhz
#define TICK_PER_US		1
#define PRESCALER_RELOAD_VAL	(FREQ_MZH / TICK_PER_US)  // reload to 1us

#define TIMER_CTRL_EN	0x1  // Enable timer
#define TIMER_CTRL_RS	0x2  // Reload automatically
#define TIMER_CTRL_LD	0x4  // Load the reload value immediately
