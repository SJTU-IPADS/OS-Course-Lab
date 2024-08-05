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

#pragma once

#include <chcore/type.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Spec from https://man.openbsd.org/FreeBSD-12.0/cpio.5 */

struct cpio_newc_header {
        char c_magic[6];
        char c_ino[8];
        char c_mode[8];
        char c_uid[8];
        char c_gid[8];
        char c_nlink[8];
        char c_mtime[8];
        char c_filesize[8];
        char c_devmajor[8];
        char c_devminor[8];
        char c_rdevmajor[8];
        char c_rdevminor[8];
        char c_namesize[8];
        char c_check[8];
};

struct cpio_header {
        char c_magic[6];
        u64 c_ino;
        u64 c_mode;
        u64 c_uid;
        u64 c_gid;
        u64 c_nlink;
        u64 c_mtime;
        u64 c_filesize;
        u64 c_devmajor;
        u64 c_devminor;
        u64 c_rdevmajor;
        u64 c_rdevminor;
        u64 c_namesize;
        u64 c_check;
};

#define cpio_fatal(fmt, ...)                                                   \
        do {                                                                   \
                fprintf(stderr, "CPIO tool fatal error: " fmt, ##__VA_ARGS__); \
                exit(-1);                                                      \
        } while (0)

#define cpio_fatal_e(fmt, ...)                              \
        do {                                                \
                fprintf(stderr,                             \
                        "CPIO tool fatal error (%s): " fmt, \
                        strerror(errno),                    \
                        ##__VA_ARGS__);                     \
                exit(-1);                                   \
        } while (0)

#define ALIGN4_UP(x) \
        ((((unsigned long)x) & (~3lu)) + ((!!(((unsigned long)x) & 3)) << 2))

struct cpio_file {
        struct cpio_header header;
        const char *name;
        const char *data;
        struct cpio_file *next;
};

void cpio_init_g_files(void);
int cpio_extract_file(const void *addr, const char *dirat);
void cpio_extract(const void *addr, const char *dirat);
int cpio_extract_single(const void *addr, const char *target,
                        int (*cpio_single_file_filler)(const void *start,
                                                       size_t size, void *data),
                        void *data);

#ifdef __cplusplus
}
#endif
