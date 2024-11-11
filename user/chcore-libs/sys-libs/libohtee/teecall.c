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

#include "teecall.h"
#include <assert.h>
#include <stdio.h>
#include <chcore/syscall.h>
#include <chcore/opentrustee/syscall.h>

int32_t tee_pull_kernel_variables(const kernel_shared_varibles_t *pVar)
{
        return usys_tee_pull_kernel_var((unsigned long)pVar);
}

void tee_push_rdr_update_addr(uint64_t addr, uint32_t size, bool is_cache_mem,
                              const char *chip_type_buff, uint32_t buff_len)
{
        (void)usys_tee_push_rdr_update_addr(
                addr, size, is_cache_mem, chip_type_buff, buff_len);
}

int debug_rdr_logitem(char *str, size_t str_len)
{
        return usys_debug_rdr_logitem(str, str_len);
}

int32_t teecall_cap_time_sync(uint32_t seconds, uint32_t mills)
{
        printf("%s not implemented!\n", __func__);
        return 0;
}