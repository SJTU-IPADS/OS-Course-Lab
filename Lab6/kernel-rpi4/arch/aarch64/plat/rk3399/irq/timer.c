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

#include <common/types.h>

u64 tick_per_us;

void plat_timer_init(void)
{
}

void plat_set_next_timer(u64 tick_delta)
{
}

void plat_handle_timer_irq(u64 tick_delta)
{
}

void plat_disable_timer(void)
{
}

void plat_enable_timer(void)
{
}

u64 plat_get_mono_time(void)
{
        return 0;
}

u64 plat_get_current_tick(void)
{
        return 0;
}
