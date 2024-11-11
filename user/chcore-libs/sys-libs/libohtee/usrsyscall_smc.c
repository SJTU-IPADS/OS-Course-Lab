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

#include <usrsyscall_smc.h>
#include <chcore/opentrustee/syscall.h>
#include <stdio.h>

int smc_wait_switch_req(struct cap_teesmc_buf *smc_buf)
{
        int ret;
        struct smc_registers regs;

        ret = usys_tee_wait_switch_req(&regs);
        if (ret < 0) {
                return ret;
        }

        smc_buf->ops = regs.x1;

        return CAP_TEESMC_OPS_NORMAL;
}

int smc_switch_req(enum cap_teesmc_req req)
{
        struct smc_registers regs;

        if (req == CAP_TEESMC_REQ_STARTTZ) {
                regs.x0 = TZ_SWITCH_REQ_ENTRY_DONE;
        } else if (req == CAP_TEESMC_REQ_IDLE) {
                regs.x0 = TZ_SWITCH_REQ_STD_RESPONSE;
                regs.x1 = 0;
        } else {
                return -1;
        }

        return usys_tee_switch_req(&regs);
}

void init_teesmc_hdlr(void)
{
        printf("%s not implemented!\n", __func__);
}
