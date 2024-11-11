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

#ifndef FILE_NAMESPACE_H
#define FILE_NAMESPACE_H

#include <string.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MNT_READONLY	(0x00)
#define MNT_READWRITE	(0x01)
#define FS_NS_MAX_PATH_LEN 63
#define FS_NS_MAX_MOUNT_NUM 5

struct user_file_info_node {
        char path[FS_NS_MAX_PATH_LEN + 1];
        size_t path_len;
        char mount_path[FS_NS_MAX_PATH_LEN + 1]; //path in the whole content tree
        size_t mount_path_len;
        int flags;
};

struct user_file_namespace {
        struct user_file_info_node file_infos[FS_NS_MAX_MOUNT_NUM];
        int nums;
};

int fs_ns_add_config_item(struct user_file_namespace *fs_ns,
                          const char *source,
                          const char *target,
                          int flags);

void fs_ns_init(struct user_file_namespace *fs_ns);

#ifdef __cplusplus
}
#endif

#endif /* FILE_NAMESPACE_H */
