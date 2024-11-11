# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

chcore_enable_clang_tidy(
    EXTRA_CHECKS -bugprone-implicit-widening-of-multiplication-result)
add_library(fat32_3rd_party OBJECT ffunicode.c ff.c)
chcore_enable_clang_tidy()
add_user_executable(fat32.srv diskio.c ff_server.c ffsystem.c fat32_ops.c
                         fat32_utils.c)
target_link_libraries(fat32.srv PRIVATE fs_base fat32_3rd_party)
chcore_copy_all_targets_to_ramdisk()
chcore_add_configurable_server(server_config.yaml)
