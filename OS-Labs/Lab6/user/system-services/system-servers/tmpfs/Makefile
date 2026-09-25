# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

CFLAGS := $(DEFAULT_USER_CFLAGS) -Iinclude -I$(SERVER_DIR)/fs_base
LIBS := fs_base
LDFLAGS := $(DEFAULT_STATIC_USER_LDFLAGS) -pie -L$(SERVER_DIR)/fs_base $(addprefix -l,$(LIBS))
SRCS := $(wildcard *.c)
SRCS += incbin_ramdisk.S
OBJS := $(SRCS:%=%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: ramdisk.cpio
ramdisk.cpio:
	$(Q)rm -rf ramdisk-dir
	$(Q)cp -r $(ROOT_DIR)/ramdisk-dir .
	$(Q)cd ramdisk-dir && find . ! -name ramdisk.cpio | cpio -o -H newc > ../$@

incbin_ramdisk.S: incbin_ramdisk.tpl.S ramdisk.cpio
	$(Q)sed 's/$${binary_path}/ramdisk.cpio/g' $< > $@

tmpfs.srv: $(OBJS)
	$(Q)$(CC) $(LDFLAGS) $^ -o $@

clean:
	$(Q)rm -rf $(OBJS) $(DEPS) tmpfs.srv ramdisk.cpio incbin_ramdisk.S ramdisk-dir

-include $(DEPS)

$(OBJS): %.o: %
	$(Q)$(CC) $(CFLAGS) -c $< -o $@ -MMD
