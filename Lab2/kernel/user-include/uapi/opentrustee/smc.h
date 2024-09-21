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

#ifndef UAPI_OPENTRUSTEE_SMC_H
#define UAPI_OPENTRUSTEE_SMC_H

struct smc_registers {
        unsigned long x0;
        unsigned long x1;
        unsigned long x2;
        unsigned long x3;
        unsigned long x4;
};

enum tz_switch_req {
        TZ_SWITCH_REQ_ENTRY_DONE,
        TZ_SWITCH_REQ_STD_REQUEST,
        TZ_SWITCH_REQ_STD_RESPONSE,
        TZ_SWITCH_REQ_NR
};

#endif /* UAPI_OPENTRUSTEE_SMC_H */
