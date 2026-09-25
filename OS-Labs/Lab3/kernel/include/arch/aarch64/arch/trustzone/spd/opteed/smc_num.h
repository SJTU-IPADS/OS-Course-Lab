/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#ifndef ARCH_AARCH64_ARCH_TRUSTZONE_SPD_OPTEED_SMC_NUM_H
#define ARCH_AARCH64_ARCH_TRUSTZONE_SPD_OPTEED_SMC_NUM_H

#define SMC_ENTRY_DONE   0xbe000000
#define SMC_STD_REQUEST  0x3e000008
#define SMC_STD_RESPONSE 0xbe000005
#define SMC_ON_DONE      0xbe000001
#define SMC_OFF_DONE     0xbe000002
#define SMC_SUSPEND_DONE 0xbe000003
#define SMC_RESUME_DONE  0xbe000004
#define SMC_FIQ_DONE     0xbe000006
#define SMC_SYSOFF_DONE  0xbe000007
#define SMC_SYSRST_DONE  0xbe000008

#endif /* ARCH_AARCH64_ARCH_TRUSTZONE_SPD_OPTEED_SMC_NUM_H */
