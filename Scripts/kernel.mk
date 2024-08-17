V ?= 0
Q := @

ifeq ($(V), 1)
	Q :=
endif

BUILD_DIR := $(CURDIR)/build
KERNEL_IMG := $(BUILD_DIR)/kernel.img
QEMU := qemu-system-aarch64
_QEMU := $(CURDIR)/scripts/qemu/qemu_wrapper.sh $(QEMU)
QEMU_GDB_PORT := 1234
QEMU_OPTS := -machine raspi3b -nographic -serial mon:stdio -m size=1G -kernel $(KERNEL_IMG)
CHBUILD := $(CURDIR)/chbuild

export PROJECT CURDIR LAB

all: build

defconfig:
	$(Q)$(CHBUILD) defconfig

build:
	$(Q)test -f $(CURDIR)/.config || $(CHBUILD) defconfig
	$(Q)$(CHBUILD) build

clean:
	$(Q)$(CHBUILD) clean

distclean:
	$(Q)$(CHBUILD) distclean

qemu:
	$(Q)$(_QEMU) $(QEMU_OPTS)

qemu-gdb:
	$(Q)$(_QEMU) -S -gdb tcp::$(QEMU_GDB_PORT) $(QEMU_OPTS)

gdb:
	$(Q)$(GDB) --nx -x $(CURDIR)/.gdbinit

grade:
	$(Q)test -f $(CURDIR)/.config && cp $(CURDIR)/.config $(CURDIR)/.config.bak
	$(Q)$(PROJECT)/Scripts/grader.sh
	$(Q)test -f $(CURDIR)/.config.bak && mv $(CURDIR)/.config.bak $(CURDIR)/.config

.PHONY: qemu qemu-gdb gdb defconfig build clean distclean grade all
