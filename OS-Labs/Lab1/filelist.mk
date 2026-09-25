BOOT   := kernel/arch/aarch64/boot/raspi3

FILES  := $(BOOT)/init/tools.S \
		  $(BOOT)/peripherals/uart.c \
		  $(BOOT)/init/mmu.c
