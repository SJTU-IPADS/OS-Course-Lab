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
LDFLAGS := $(DEFAULT_STATIC_USER_LDFLAGS) -pie
SRCS := $(wildcard *.c)
OBJS := $(SRCS:%=%.o)
DEPS := $(OBJS:.o=.d)

fsm.srv: $(OBJS)
	$(Q)$(CC) $(LDFLAGS) $^ -o $@

clean:
	$(Q)rm -f $(OBJS) $(DEPS) fsm.srv

-include $(DEPS)

$(OBJS): %.o: %
	$(Q)$(CC) $(CFLAGS) -c $< -o $@ -MMD
