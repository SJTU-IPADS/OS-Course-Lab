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

#include <chcore/file_namespace.h>

int fs_ns_add_config_item(struct user_file_namespace *fs_ns,
                          const char *source,
                          const char *target,
                          int flags)
{
        if (fs_ns->nums >= FS_NS_MAX_MOUNT_NUM)
                return -EOVERFLOW;

        if (strlen(target) > FS_NS_MAX_PATH_LEN
            || strlen(source) > FS_NS_MAX_PATH_LEN)
                return -ENAMETOOLONG;

        strcpy(fs_ns->file_infos[fs_ns->nums].path, target);
        fs_ns->file_infos[fs_ns->nums].path_len = strlen(target);
        strcpy(fs_ns->file_infos[fs_ns->nums].mount_path, source);
        fs_ns->file_infos[fs_ns->nums].mount_path_len = strlen(source);
        fs_ns->file_infos[fs_ns->nums].flags = flags;
        fs_ns->nums++;

        return 0;
}

void fs_ns_init(struct user_file_namespace *fs_ns)
{
        fs_ns->nums = 0;
}
