OBJECTS := kernel/object

CAP_GROUP := $(OBJECTS)/cap_group.c
THREAD := $(OBJECTS)/thread.c

SCHED_ARCH := kernel/arch/aarch64/sched
CONTEXT := $(SCHED_ARCH)/context.c

IRQ_ARCH := kernel/arch/aarch64/irq
IRQ_ENTRY_S := $(IRQ_ARCH)/irq_entry.S

STDIO := user/chcore-libc/libchcore/porting/overrides/src/chcore-port/stdio.c

HELLO := ramdisk/hello_world.bin
FILES := $(CAP_GROUP) \
		 $(THREAD) \
		 $(CONTEXT) \
		 $(IRQ_ENTRY_S) \
		 $(STDIO) \
		 $(HELLO)
