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

#include <stdio.h>
#include <chcore/type.h>
#include <chcore/syscall.h>

#define HIKEY960_TIMER_IRQ 0x1b
#define X86_TIMER_IRQ      0x20

int main(int argc, char *argv[])
{
        cap_t irq_cap;
        int irq_num;

        if (argc < 2) {
                printf("Usage: %s [irq number]\n", argv[0]);
                return -1;
        }
        irq_num = atoi(argv[1]);
        printf("irq number is %d\n", irq_num);
        
        irq_cap = usys_irq_register(irq_num);
        // irq_cap = usys_irq_register(HIKEY960_TIMER_IRQ);
        // irq_cap = usys_irq_register(X86_TIMER_IRQ);
        while (1) {
                printf("Waiting irq\n");
                usys_irq_wait(irq_cap, true);
                printf("Waked up. Acking irq\n");
                usys_irq_ack(irq_cap);
        }

        return 0;
}
