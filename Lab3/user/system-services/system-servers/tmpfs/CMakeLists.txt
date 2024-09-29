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
set(binary_path ${CHCORE_RAMDISK_DIR}/ramdisk.cpio)
configure_file(incbin_ramdisk.tpl.S incbin_ramdisk.S)
unset(binary_path)

add_library(fs_base STATIC IMPORTED)
set_target_properties(fs_base PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libs/libfs_base.a")
add_library(cpio STATIC IMPORTED)
set_target_properties(cpio PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libs/libcpio.a")
add_executable(tmpfs.srv tmpfs.c.obj internal_ops.c.obj namei.c.obj mem_usage_tool.c.obj main.c.obj)

add_custom_target(touch-inc-ramdisk
                COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/incbin_ramdisk.S)

add_dependencies(tmpfs.srv ramdisk touch-inc-ramdisk)

target_sources(tmpfs.srv PRIVATE incbin_ramdisk.S)

target_link_libraries(tmpfs.srv PRIVATE fs_base cpio)

chcore_all_force_static_linked()
