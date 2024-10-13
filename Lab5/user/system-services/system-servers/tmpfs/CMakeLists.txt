# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

add_link_options(-static)
chcore_enable_clang_tidy(
    EXTRA_CHECKS -performance-no-int-to-ptr,-clang-analyzer-deadcode.DeadStores)
message("CMAKE_C_CLANG_TIDY: ${CMAKE_C_CLANG_TIDY}")
set(binary_path ${CHCORE_RAMDISK_DIR}/ramdisk.cpio)
configure_file(incbin_ramdisk.tpl.S incbin_ramdisk.S)
unset(binary_path)
add_library(cpio STATIC cpio.c cpio.h)
add_executable(tmpfs.srv tmpfs.c internal_ops.c namei.c mem_usage_tool.c main.c)

add_custom_target(touch-inc-ramdisk
                COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/incbin_ramdisk.S)

add_dependencies(tmpfs.srv ramdisk touch-inc-ramdisk)

target_sources(tmpfs.srv PRIVATE incbin_ramdisk.S)

target_link_libraries(tmpfs.srv PRIVATE fs_base cpio)

chcore_all_force_static_linked()
