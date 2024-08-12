# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

# Q=@
Q=

include ../config.mk

CC = $(CHCORE_COMPILER)
# XXX: -march=armv8-a+nofp -fno-stack-protector are plat specific
CFLAGS = -target $(CHCORE_CROSS_COMPILE) -Wall -Wno-initializer-overrides -Wno-asm-operand-widths\
                 -fheinous-gnu-extensions -nostdinc -ffreestanding -march=armv8-a+nofp -fno-stack-protector \
		 -fno-pic -fno-pie -mcmodel=large -O3
CFLAGS += -Iinclude -Iinclude/arch/${CHCORE_ARCH} \
		  -Iinclude/arch/${CHCORE_ARCH}/plat/${CHCORE_PLAT} \
          -Iinclude/arch/${CHCORE_ARCH}/arch/trustzone/spd/${CHCORE_SPD} \
		  -Iarch/${CHCORE_ARCH}/boot/${CHCORE_PLAT}/include \
		  -Iuser-include
CFLAGS += $(CONFIG_FLAGS)
CFLAGS += -DLOG_LEVEL=1

LDFLAGS =-target $(CHCORE_CROSS_COMPILE)  -no-pie -nostdlib -nostartfiles \
		 -Wl,--build-id=none,--no-dynamic-linker -fuse-ld=lld

OBJCOPY := llvm-objcopy

ARCH_DIRS := backtrace irq machine mm sched sync trustzone debug
ARCH_PATH := arch/$(CHCORE_ARCH)
PLAT_DIRS := irq mm uart
PLAT_PATH := arch/$(CHCORE_ARCH)/plat/$(CHCORE_PLAT)
SPD_PATH := arch/$(CHCORE_ARCH)/trustzone/spd/$(CHCORE_SPD)
DIRS := ipc irq lib mm object sched syscall \
		$(ARCH_PATH) $(addprefix $(ARCH_PATH)/,$(ARCH_DIRS)) \
        $(PLAT_PATH) $(addprefix $(PLAT_PATH)/,$(PLAT_DIRS)) \
        $(SPD_PATH)
C_SRCS := $(foreach dir,$(DIRS),$(wildcard $(dir)/*.c))
C_OBJS := $(C_SRCS:%.c=%.c.o)
ASM_SRCS += $(foreach dir,$(DIRS),$(wildcard $(dir)/*.S))
ASM_SRCS += incbin_promgr_bin.S
ASM_OBJS := $(ASM_SRCS:%.S=%.S.o)

INIT_C_SRCS := $(wildcard arch/$(CHCORE_ARCH)/boot/$(CHCORE_PLAT)/init/*.c)
INIT_C_OBJS := $(INIT_C_SRCS:%.c=%.c.o)
INIT_ASM_SRCS := $(wildcard arch/$(CHCORE_ARCH)/boot/$(CHCORE_PLAT)/init/*.S) \
                 $(wildcard arch/$(CHCORE_ARCH)/trustzone/spd/$(CHCORE_SPD)/init/*.S)
INIT_ASM_OBJS := $(INIT_ASM_SRCS:%.S=%.S.o)

ALL_OBJS := $(C_OBJS) $(ASM_OBJS) $(INIT_C_OBJS) $(INIT_ASM_OBJS)
ALL_DEPS := $(ALL_OBJS:.o=.d)

.PHONY: all clean
all: bl32.bin

# XXX: Strip off .interp since it is before init section.
#      Not sure if other sections can also be placed here.
bl32.bin: kernel.img
	$(Q)$(OBJCOPY) -O binary -S $< $@

kernel.img: $(ALL_OBJS) linker.ld
	$(Q)$(CC) -Wl,-T,linker.ld $(LDFLAGS) $(ALL_OBJS) -o $@

-include $(ALL_DEPS)

clean:
	$(Q)rm -f $(ALL_OBJS) $(ALL_DEPS) linker.ld linker.ld.S kernel.img bl32.bin trust.img incbin_promgr_bin.S

$(C_OBJS) $(INIT_C_OBJS): %.o: %
	$(Q)$(CC) $(CFLAGS) -c $< -o $@ -MMD

$(ASM_OBJS) $(INIT_ASM_OBJS): %.o: %
	$(Q)$(CC) $(CFLAGS) -D__ASM__ -c $< -o $@

INIT_OBJS :=  $(INIT_ASM_OBJS) $(INIT_C_OBJS)
# move start.S to the front for binary format
INIT_OBJS := $(filter %start.S.o, $(INIT_OBJS)) $(filter-out %start.S.o, $(INIT_OBJS))
INIT_OBJS_LIST := $(subst /,\/,$(INIT_OBJS))
linker.ld: $(ARCH_PATH)/boot/linker.tpl.ld
	$(Q)sed -e 's/$${init_objects}/$(INIT_OBJS_LIST)/g' -e 's/$${incbin_linker_script}/*(.incbin-procmgr_bin)\n        . = ALIGN(4K);/g' $< > linker.ld.S
	$(Q)$(CC) -E -x c -I $(ARCH_PATH)/boot/$(CHCORE_PLAT)/include linker.ld.S | grep -v "^#"  > linker.ld

incbin_promgr_bin.S: incbin.tpl.S
	$(Q)sed 's/$${binary_name}/procmgr_bin/g;s/$${binary_path}/$(subst /,\/,$(PROCMGR))/g' $< > $@
