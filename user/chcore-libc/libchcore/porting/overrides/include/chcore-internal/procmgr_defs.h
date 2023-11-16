/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef PROCMGR_DEFS_H
#define PROCMGR_DEFS_H

#include <chcore/ipc.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum PROC_REQ {
        PROC_REQ_NEWPROC = 1,
        PROC_REQ_WAIT,
        PROC_REQ_GET_MT_CAP,
        PROC_REQ_GET_SERVER_CAP,
        PROC_REQ_RECV_SIG,
        PROC_REQ_CHECK_STATE,
        PROC_REQ_GET_SHELL_CAP,
        PROC_REQ_SET_SHELL_CAP,
        PROC_REQ_GET_TERMINAL_CAP,
        PROC_REQ_SET_TERMINAL_CAP,
        PROC_REQ_KILL,
        PROC_REQ_GET_SYSTEM_INFO,
        PROC_REQ_MAX
};

enum PROC_TYPE {
        SYSTEM_SERVER = 1,
        SYSTEM_DRIVER,
        COMMON_APP,
        TRACED_APP,
};

enum CONFIGURABLE_SERVER {
	SERVER_TMPFS = 1,
	SERVER_SYSTEMV_SHMMGR,
	SERVER_HDMI_DRIVER,
	SERVER_SD_CARD,
	SERVER_FAT32_FS,
	SERVER_EXT4_FS,
	SERVER_USB_DEVMGR,
	SERVER_SERIAL,
	SERVER_GPIO,
	SERVER_GUI,
	SERVER_LITTLEFS,
	SERVER_FLASH,
	CONFIG_SERVER_MAX,
};

#define PROC_REQ_NAME_LEN      255
#define PROC_REQ_TEXT_SIZE     1600
#define PROC_REQ_ARGC_MAX      128
#define PROC_REQ_ENVC_MAX      128
#define PROC_REQ_ENV_TEXT_SIZE 256

struct proc_request {
        enum PROC_REQ req;
        union {
                struct {
                        int argc;
                        char argv_text[PROC_REQ_TEXT_SIZE];
                        off_t argv_off[PROC_REQ_ARGC_MAX];
                        int proc_type;
                } newproc;
                struct {
                        pid_t pid;
                        int exitstatus;
                } wait;
                struct {
                        pid_t pid;
                } get_mt_cap;
                struct {
                        enum CONFIGURABLE_SERVER server_id;
                } get_server_cap;
                struct {
                        char sig;
                } recv_sig;
                struct {
                        pid_t pid;
                } check_state;
                struct {
                        /* no args */
                } get_shell_cap;
                struct {
                        /* no args */
                } set_shell_cap;
                struct {
                        /* no args */
                } get_terminal_cap;
                struct {
                        /* no args */
                } set_terminal_cap;
                struct {
                        pid_t pid;
                } kill;
        };
};

#ifdef __cplusplus
}
#endif

#endif /* PROCMGR_DEFS_H */
