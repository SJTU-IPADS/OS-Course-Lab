/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#ifndef ARCH_AARCH64_ARCH_TRUSTZONE_TLOGGER_H
#define ARCH_AARCH64_ARCH_TRUSTZONE_TLOGGER_H

#include <common/types.h>
#include <common/tee_uuid.h>

#define UUID_LEN              16
#define NEVER_USED_LEN        32U
#define LOG_ITEM_RESERVED_LEN 1U

/* 64 byte head + user log */
struct log_item {
        u8 never_used[NEVER_USED_LEN];
        u16 magic;
        u16 reserved0;
        u32 serial_no;
        s16 real_len; /* log real len */
        u16 buffer_len; /* log buffer's len, multiple of 32 bytes */
        u8 uuid[UUID_LEN];
        u8 log_source_type;
        u8 reserved[LOG_ITEM_RESERVED_LEN];
        u8 log_level;
        u8 new_line; /* '\n' char, easy viewing log in bbox.bin file */
        u8 log_buffer[0];
};

/* --- for log mem --------------------------------- */
#define LOG_BUFFER_RESERVED_LEN 11U
#define VERSION_INFO_LEN        156U

/*
 * Log's buffer flag info, size: 64 bytes head + 156 bytes's version info.
 * For filed description:
 * last_pos : current log's end position, last log's start position.
 * write_loops: Write cyclically. Init value is 0, when memory is used
 *              up, the value add 1.
 */
struct log_buffer_flag {
        u32 reserved0;
        u32 last_pos;
        u32 write_loops;
        u32 log_level;
        u32 reserved[LOG_BUFFER_RESERVED_LEN];
        u32 max_len;
        u8 version_info[VERSION_INFO_LEN];
};

struct log_buffer {
        struct log_buffer_flag flag;
        u8 buffer_start[0];
};

void enable_tlogger(void);
bool is_tlogger_on(void);

int tmp_tlogger_init(void);

int append_chcore_log(const char *str, size_t len, bool is_kernel);

int ot_sys_tee_push_rdr_update_addr(paddr_t addr, size_t size, bool is_cache_mem,
                                 char *chip_type_buff, size_t buff_len);

int ot_sys_debug_rdr_logitem(char *str, size_t str_len);

#endif /* ARCH_AARCH64_ARCH_TRUSTZONE_TLOGGER_H */
