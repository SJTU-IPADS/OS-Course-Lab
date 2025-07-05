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

#ifdef CHCORE_OPENTRUSTEE
#include "opentrustee_defs.h"
#endif /* CHCORE_OPENTRUSTEE */

#ifdef __cplusplus
extern "C" {
#endif

enum PROC_REQ {
        PROC_REQ_NEWPROC = 1,
        PROC_REQ_WAIT,
        PROC_REQ_GET_MT_CAP,
        PROC_REQ_GET_SERVICE_CAP,
        PROC_REQ_SET_SERVICE_CAP,
        PROC_REQ_RECV_SIG,
        PROC_REQ_CHECK_STATE,
        PROC_REQ_GET_SHELL_CAP,
        PROC_REQ_SET_SHELL_CAP,
        PROC_REQ_GET_TERMINAL_CAP,
        PROC_REQ_SET_TERMINAL_CAP,
        PROC_REQ_KILL,
#ifdef CHCORE_OPENTRUSTEE
        PROC_REQ_SPAWN,
        PROC_REQ_INFO_PROC_BY_PID,
        PROC_REQ_TRANSFER_PMO_CAP,
        PROC_REQ_TEE_ALLOC_SHM,
        PROC_REQ_TEE_GET_SHM,
        PROC_REQ_TEE_FREE_SHM,
#endif /* CHCORE_OPENTRUSTEE */
        PROC_REQ_GET_SYSTEM_INFO,
        PROC_REQ_MAX
};

enum PROC_TYPE {
        SYSTEM_SERVER = 1,
        SYSTEM_DRIVER,
        COMMON_APP,
        TRACED_APP,
};
#if 0
enum CONFIGURABLE_SERVER {
        SERVER_TMPFS = 1,
        SERVER_SYSTEMV_SHMMGR,
        SERVER_HDMI_DRIVER,
        SERVER_SD_CARD,
        SERVER_FAT32_FS,
        SERVER_EXT4_FS,
        SERVER_USB_DEVMGR,
        SERVER_AUDIO,
        SERVER_CPU_DRIVER,
        SERVER_SERIAL,
        SERVER_GPIO,
        SERVER_GUI,
        SERVER_CHANMGR,
        SERVER_LITTLEFS,
        SERVER_FLASH,
        CONFIG_SERVER_MAX,
};
#else 
#define SERVER_TMPFS "tmpfs"
#define SERVER_SYSTEMV_SHMMGR "systemv_shmmgr"
#define SERVER_HDMI_DRIVER "hdmi"
#define SERVER_SD_CARD "sd_card"
#define SERVER_FAT32_FS "fat32"
#define SERVER_EXT4_FS "ext4"
#define SERVER_USB_DEVMGR "usb_devmgr"
#define SERVER_AUDIO "audio"
#define SERVER_CPU_DRIVER "cpu_driver"
#define SERVER_SERIAL "serial"
#define SERVER_GPIO "gpio"
#define SERVER_GUI "gui"
#define SERVER_CHANMGR "chanmgr"
#define SERVER_LITTLEFS "littlefs"
#define SERVER_FLASH "flash"
#endif

#define PROC_REQ_NAME_LEN      255
#define PROC_REQ_TEXT_SIZE     1600
#define PROC_REQ_ARGC_MAX      128
#define PROC_REQ_ENVC_MAX      128
#define PROC_REQ_ENV_TEXT_SIZE 256

#define SERVICE_NAME_LEN 32

#ifdef CHCORE_OPENTRUSTEE
struct proc_info {
        spawn_uuid_t uuid;
        size_t stack_size;
};
#endif /* CHCORE_OPENTRUSTEE */

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
                        char service_name[SERVICE_NAME_LEN + 1];
                } get_service_cap;
                struct {
                        char service_name[SERVICE_NAME_LEN + 1];
                } set_service_cap;
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
#ifdef CHCORE_OPENTRUSTEE
                struct {
                        int argc;
                        char argv_text[PROC_REQ_TEXT_SIZE];
                        int argv_off[PROC_REQ_ARGC_MAX];
                        int envc;
                        char envp_text[PROC_REQ_ENV_TEXT_SIZE];
                        int envp_off[PROC_REQ_ENVC_MAX];
                        int attr_valid;
                        posix_spawnattr_t attr;
                        cap_t main_thread_id;
                        char path[PROC_REQ_NAME_LEN];
                } spawn;
                struct {
                        pid_t pid;
                        struct proc_info info;
                } info_proc_by_pid;
                struct {
                        uint32_t task_id;
                        cap_t pmo;
                } transfer_pmo_cap;
                struct {
                        uint32_t task_id;
                        cap_t pmo;
                } task_unmap_ns;
                struct {
                        vaddr_t vaddr;
                        uint32_t size;
                        struct tee_uuid uuid;
                } tee_alloc_shm;
                struct {
                        vaddr_t vaddr;
                        pid_t pid;
                } tee_get_shm;
                struct {
                        vaddr_t vaddr;
                } tee_free_shm;
#endif /* CHCORE_OPENTRUSTEE */
        };
};

#ifdef __cplusplus
}
#endif

#endif /* PROCMGR_DEFS_H */