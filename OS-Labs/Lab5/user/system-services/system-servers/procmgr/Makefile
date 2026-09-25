# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

CFLAGS := $(DEFAULT_USER_CFLAGS) -Iinclude
LDFLAGS := $(DEFAULT_STATIC_USER_LDFLAGS) -pie
SRCS := $(wildcard *.c)
SRCS += libs/libchcoreelf/libchcoreelf.c
SRCS += libs/liblaunch/liblaunch.c
SRCS += libs/librefcnt/librefcnt.c

# link fsm.srv and tmpfs.srv to procmgr.srv
INCBINS := fsm tmpfs
INCBINS_SRC := $(INCBINS:%=%.incbin.S)

.PHONY: fsm.srv tmpfs.srv $(INCBINS_SRC)
fsm.srv: $(SERVER_DIR)/fsm/fsm.srv
	$(Q)cp $< .
tmpfs.srv: $(SERVER_DIR)/tmpfs/tmpfs.srv
	$(Q)cp $< .

$(INCBINS_SRC): %.incbin.S: incbin_basic_srv.tpl.S %.srv
	$(Q)sed 's/$${_binary_name}/$*_elf/g;s/$${_binary_path}/$*.srv/g' $< > $@

SRCS += $(INCBINS_SRC)
OBJS := $(SRCS:%=%.o)
DEPS := $(OBJS:.o=.d)

TOOL_DIR = $(ROOT_DIR)/chcore-customtools/tools/read_procmgr_elf_tool
read_procmgr_elf_tool: $(TOOL_DIR)/elf.c $(TOOL_DIR)/main.c
	clang $^ -o $@

procmgr.srv: linker.ld $(OBJS)
	$(Q)$(CC) -Wl,-T,$< $(LDFLAGS) $(OBJS) -o $@

procmgr: procmgr.srv read_procmgr_elf_tool
	$(Q)./read_procmgr_elf_tool procmgr.srv
	$(Q)cp procmgr.srv procmgr
	$(Q)exec 3<>procmgr && cat elf_info.temp >&3
	$(Q)rm -f elf_info.temp

clean:
	$(Q)rm -f $(OBJS) $(DEPS) procmgr.srv procmgr fsm.srv tmpfs.srv $(INCBINS_SRC) read_procmgr_elf_tool

-include $(DEPS)

$(OBJS): %.o: %
	$(Q)$(CC) $(CFLAGS) -c $< -o $@ -MMD
