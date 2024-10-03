ARCH_IRQ := kernel/arch/aarch64/plat/raspi3/irq
KERNEL_IRQ := kernel/irq
SCHED := kernel/sched
IPC := kernel/ipc

SCHEDULER := $(SCHED)/sched.c
RR := $(SCHED)/policy_rr.c
ARCH_TIMER := $(ARCH_IRQ)/timer.c
IRQ := $(ARCH_IRQ)/irq.c
KERNEL_TIMER := $(IRQ)/timer.c
CONNECTION := $(IPC)/connection.c

FILES := $(SCHEDULER) \
		 $(RR) \
		 $(ARCH_TIMER) \
		 $(IRQ) \
		 $(KERNEL_TIMER) \
		 $(CONNECTION)
