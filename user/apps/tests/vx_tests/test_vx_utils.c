#include <stdio.h>
#include <sysLib.h>

#include "test_tools.h"

void test_irq_handler(void)
{
        return;
}

int main()
{
        unsigned tick;

        info("Please verify the following output is "
                "\"Hello VxWorks\\n\":\n");
        serial_puts("Hello VxWor");
        serial_putc('k');
        serial_putc('s');
        serial_putc('\n');

        Clear_Interrupt(IRQ_CAN2);
        Close_Interrupt(IRQ_CAN1);
        Bind_Interrupt(IRQ_CAN1, test_irq_handler);

        tick = Get_Gpt_Clock_Counter();
        chcore_assert(tick > 0, "tick is zero");

        info("test vx utils finished\n");

        return 0;
}