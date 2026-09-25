SYSTEM_SERVERS := user/system-services/system-servers
FS_BASE_DIR := $(SYSTEM_SERVERS)/fs_base
FSM_DIR := $(SYSTEM_SERVERS)/fsm

FS_VNODE := $(FS_BASE_DIR)/fs_vnode.c
FS_WRAPPER := $(FS_BASE_DIR)/fs_wrapper.c
FS_OPS := $(FS_BASE_DIR)/fs_wrapper_ops.c
FSM := $(FSM_DIR)/fsm.c
FSM_CLIENT_CAP := $(FSM_DIR)/fsm_client_cap.c
FS_FILE_FAULT := $(FS_BASE_DIR)/fs_page_fault.c

FILES := $(FS_VNODE) $(FS_WRAPPER) $(FS_OPS) $(FSM) $(FSM_CLIENT_CAP) $(FS_FILE_FAULT)
