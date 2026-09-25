MM := kernel/mm
MM_ARCH := kernel/arch/aarch64/mm
ARCH := kernel/arch/aarch64

BUDDY := $(MM)/buddy.c
SLAB := $(MM)/slab.c
KMALLOC := $(MM)/kmalloc.c
PTE := $(MM_ARCH)/page_table.c 
MAIN := $(ARCH)/main.c
PGFAULT := $(ARCH)/irq/pgfault.c $(MM)/pgfault_handler.c
VMSPACE := $(MM)/vmspace.c
CONFIG := kernel/config.cmake

FILES := $(BUDDY) $(KMALLOC) $(SLAB) $(PTE) $(MAIN) $(PGFAULT) $(VMSPACE) $(CONFIG)
