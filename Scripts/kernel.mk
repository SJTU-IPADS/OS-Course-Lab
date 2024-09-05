V ?= 0
Q := @

ifeq ($(V), 1)
	Q :=
endif

BUILDDIR := $(LABDIR)/build
KERNEL_IMG := $(BUILDDIR)/kernel.img
_QEMU := $(LABDIR)/scripts/qemu/qemu_wrapper.sh $(QEMU)
QEMU_GDB_PORT := 1234
QEMU_OPTS := -machine raspi3b -nographic -serial mon:stdio -m size=1G -kernel $(KERNEL_IMG)
CHBUILD := $(SCRIPTS)/chbuild

export LABROOT LABDIR LAB TIMEOUT

all: build

defconfig:
	$(Q)$(CHBUILD) defconfig

build:
	$(Q)test -f $(LABDIR)/.config || $(CHBUILD) defconfig
	$(Q)$(CHBUILD) build

clean:
	$(Q)$(CHBUILD) clean

distclean:
	$(Q)$(CHBUILD) distclean

qemu: build
	$(Q)$(_QEMU) $(QEMU_OPTS)

qemu-gdb: build
	$(Q)echo "[QEMU] Waiting for GDB Connection"
	$(Q)$(_QEMU) -S -gdb tcp::$(QEMU_GDB_PORT) $(QEMU_OPTS)

gdb:
	$(Q)$(GDB) --nx -x $(LABDIR)/.gdbinit

grade:
	$(MAKE) distclean
	$(Q)$(GRADER) -t $(TIMEOUT) -f $(LABDIR)/scores.json make qemu

.PHONY: qemu qemu-gdb gdb defconfig build clean distclean grade all
