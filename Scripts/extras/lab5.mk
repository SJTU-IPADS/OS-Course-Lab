V ?= 0
Q := @

ifeq ($(V), 1)
	Q :=
endif

GRADER := $(SCRIPTS)/extras/lab5/grader.sh
BUILDDIR := $(LABDIR)/build
KERNEL_IMG := $(BUILDDIR)/kernel.img
_QEMU := $(SCRIPTS)/qemu_wrapper.sh $(QEMU)
QEMU_GDB_PORT := 1234
QEMU_OPTS := -machine raspi3b -nographic -serial mon:stdio -m size=1G -kernel $(KERNEL_IMG)
CHBUILD := $(SCRIPTS)/chbuild
SERIAL := $(shell tr -dc A-Za-z0-9 </dev/urandom | head -c 13; echo)

export LABROOT LABDIR SCRIPTS LAB TIMEOUT

all: build

defconfig:
	$(Q)$(CHBUILD) defconfig

build:
	$(Q)test -f $(LABDIR)/.config || $(CHBUILD) defconfig
	$(Q)$(CHBUILD) build
	$(Q)find $(LABDIR) -path */compile_commands.json \
       ! -path $(LABDIR)/compile_commands.json -print \
	   | $(SCRIPTS)/merge_compile_commands.py

clean:
	$(Q)$(CHBUILD) clean

distclean:
	$(Q)$(CHBUILD) distclean

qemu: build
	$(Q)$(_QEMU) $(QEMU_OPTS)

qemu-grade:
	$(Q)$(CHBUILD) clean
	$(MAKE) build
	$(Q)$(_QEMU) $(QEMU_OPTS)

qemu-gdb: build
	$(Q)echo "[QEMU] Waiting for GDB Connection"
	$(Q)$(_QEMU) -S -gdb tcp::$(QEMU_GDB_PORT) $(QEMU_OPTS)

gdb:
	$(Q)$(GDB) --nx -x $(SCRIPTS)/gdb/gdbinit

grade:
	$(Q)(test -f ${LABDIR}/.config && cp ${LABDIR}/.config ${LABDIR}/.config.bak) || :
	$(Q)$(DOCKER_RUN) $(GRADER) -t $(TIMEOUT)
	$(Q)(test -f ${LABDIR}/.config.bak && cp ${LABDIR}/.config.bak ${LABDIR}/.config && rm .config.bak) || :

.PHONY: qemu qemu-gdb gdb defconfig build clean distclean grade all
