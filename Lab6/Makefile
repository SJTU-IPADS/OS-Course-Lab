LAB := 5

V := @
PROJECT_DIR := .
BUILD_DIR := $(PROJECT_DIR)/build
KERNEL_IMG := $(BUILD_DIR)/kernel.img
QEMU := qemu-system-aarch64
_QEMU := $(PROJECT_DIR)/scripts/qemu/qemu_wrapper.sh $(QEMU)
QEMU_GDB_PORT := 1234
KEYBOARD_PORT := 10010
MOUSE_PORT := 10011
QEMU_OPTS := -machine raspi4b -serial null -serial mon:stdio -serial tcp::$(KEYBOARD_PORT),server,nowait -serial tcp::$(MOUSE_PORT),server,nowait -m size=2G -kernel $(KERNEL_IMG)
GDB := gdb-multiarch
CHBUILD := $(PROJECT_DIR)/chbuild
QEMU_IMG := ipads/chcore_lab_qemu:v1.0
DOCKER_RUN_CMD := docker run \
	--rm \
	-e DISPLAY=$(DISPLAY) \
	-u $$(id -u $${USER}):$$(id -g $${USER}) \
	-i -t \
	-v /tmp/.X11-unix:/tmp/.X11-unix \
	-v $$(pwd):$$(pwd) \
	-w $$(pwd) \
	--network=host \
	$(QEMU_IMG) \

.PHONY: all
all: rpi4-qemu

.PHONY: defconfig clean distclean

defconfig:
	$(V)$(CHBUILD) defconfig

rpi4-qemu:
	$(V)rm -rf $(PROJECT_DIR)/kernel
	$(V)cp -r $(PROJECT_DIR)/kernel-rpi4 $(PROJECT_DIR)/kernel
	$(V)rm -rf $(PROJECT_DIR)/ramdisk
	$(V)cp -r $(PROJECT_DIR)/ramdisk-rpi4 $(PROJECT_DIR)/ramdisk
	$(V)sed -i '2s/.*/  filename: \/circle_qemu.srv/' $(PROJECT_DIR)/ramdisk/server_configure.yaml
	$(V)sed -i '7s/.*/# - audio/' $(PROJECT_DIR)/ramdisk/server_configure.yaml
	$(V)test -f $(PROJECT_DIR)/.config || $(CHBUILD) defconfig raspi4
	$(V)$(CHBUILD) build

rpi4-board:
	$(V)rm -rf $(PROJECT_DIR)/kernel
	$(V)cp -r $(PROJECT_DIR)/kernel-rpi4 $(PROJECT_DIR)/kernel
	$(V)rm -rf $(PROJECT_DIR)/ramdisk
	$(V)cp -r $(PROJECT_DIR)/ramdisk-rpi4 $(PROJECT_DIR)/ramdisk
	$(V)sed -i '2s/.*/  filename: \/circle_onboard.srv/' $(PROJECT_DIR)/ramdisk/server_configure.yaml
	$(V)sed -i '7s/.*/  - audio/' $(PROJECT_DIR)/ramdisk/server_configure.yaml
	$(V)test -f $(PROJECT_DIR)/.config || $(CHBUILD) defconfig raspi4
	$(V)$(CHBUILD) build

rpi3-board:
	$(V)rm -rf $(PROJECT_DIR)/kernel
	$(V)cp -r $(PROJECT_DIR)/kernel-rpi3 $(PROJECT_DIR)/kernel
	$(V)rm -rf $(PROJECT_DIR)/ramdisk
	$(V)cp -r $(PROJECT_DIR)/ramdisk-rpi3 $(PROJECT_DIR)/ramdisk
	$(V)test -f $(PROJECT_DIR)/.config || $(CHBUILD) defconfig raspi3
	$(V)$(CHBUILD) build

clean:
	$(V)$(CHBUILD) clean
	$(V)rm -rf $(PROJECT_DIR)/kernel
	$(V)rm -rf $(PROJECT_DIR)/ramdisk	

distclean:
	$(V)$(CHBUILD) distclean
	$(V)rm -rf $(PROJECT_DIR)/kernel
	$(V)rm -rf $(PROJECT_DIR)/ramdisk	

.PHONY: qemu qemu-gdb gdb

qemu:	
	$(V)xhost +
	$(V)$(DOCKER_RUN_CMD) $(QEMU_OPTS)

qemu-gdb:
	$(V)$(_QEMU) -S -gdb tcp::$(QEMU_GDB_PORT) $(QEMU_OPTS)

gdb:
	$(V)$(GDB) -x $(PROJECT_DIR)/.gdbinit

