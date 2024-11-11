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

/*
 * 0: print nothing
 * 1: print ext4_srv_dbg_base
 * 2: print ext4_srv_dbg_base and ext4_srv_dbg
 */
#define EXT_SERVER_DEBUG 0

#if EXT_SERVER_DEBUG >= 1
#define ext4_srv_dbg_base(fmt, ...) \
        printf("<%s:%d>: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define ext4_srv_dbg_base(fmt, ...) \
        do {                        \
        } while (0)
#endif

#if EXT_SERVER_DEBUG >= 2
#define ext4_srv_dbg(fmt, ...) \
        printf("<%s:%d>: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define ext4_srv_dbg(fmt, ...) \
        do {                   \
        } while (0)
#endif
