# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

cmake_minimum_required(VERSION 3.14)
project(ChCoreSystemServices ASM C)

# find_package(chcore-libs REQUIRED)

set(custom_ramdisk_dir ${CHCORE_PROJECT_DIR}/ramdisk)
set(system_services_build_dir ${CMAKE_CURRENT_BINARY_DIR})

# if(CHCORE_MINI)
# add_custom_target(
#     ramdisk ALL
#     WORKING_DIRECTORY ${CHCORE_RAMDISK_DIR}
#     # install libc.so
#     # COMMAND cp ${CHCORE_MUSL_LIBC_INSTALL_DIR}/lib/libc.so .
#     # # install CI test configs
#     COMMAND cp ${CHCORE_CI_CONFIG_DIR}/*.config .
#     # install packages installed by ChPM
#     COMMAND ${_copy_chpm_install_cmd}
#     # install custom ramdisk content
#     COMMAND [ -d ${custom_ramdisk_dir} ] && cp -rL ${custom_ramdisk_dir}/* .
#             || echo "No custom ramdisk found, skipping"
#     # archive ramdisk into ramdisk.cpio
#     COMMAND find . ! -name ramdisk.cpio | cpio -o -H newc > ${CHCORE_RAMDISK_DIR}/ramdisk.cpio
#     DEPENDS)
# else()
add_custom_target(
    ramdisk ALL
    WORKING_DIRECTORY ${CHCORE_RAMDISK_DIR}
    # install libc.so
    COMMAND cp ${CHCORE_MUSL_LIBC_INSTALL_DIR}/lib/libc.so .
    # # install CI test configs
    # COMMAND cp ${CHCORE_CI_CONFIG_DIR}/*.config .
    # install packages installed by ChPM
    COMMAND ${_copy_chpm_install_cmd}
    # add nameserver for chcore
    COMMAND [ ! -d "etc" ] && mkdir etc && echo "nameserver 223.5.5.5" > etc/resolv.conf || true
    # install custom ramdisk content
    COMMAND [ -d ${custom_ramdisk_dir} ] && cp -rL ${custom_ramdisk_dir}/* .
            || echo "No custom ramdisk found, skipping"
    # archive ramdisk into ramdisk.cpio
    COMMAND find . ! -name ramdisk.cpio | cpio -o -H newc > ${CHCORE_RAMDISK_DIR}/ramdisk.cpio
    DEPENDS)
# endif()

include(CommonTools)
include(SystemServiceTools)

chcore_dump_cmake_vars()

add_compile_options(-Wall)
add_compile_options(-Werror)
string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

add_custom_target(system-services-clean)

add_subdirectory(system-servers)

chcore_get_all_targets(_targets)
set(_clean_targets)
foreach(_target ${_targets})
    if(${_target} MATCHES "-clean$")
        list(APPEND _clean_targets ${_target})
    endif()
endforeach()
add_custom_target(custom-clean DEPENDS ${_clean_targets})

# Rebuild tmpfs & procmgr when ramdisk changes
# add_custom_target(buildram DEPENDS tmpfs.srv create-procmgr-bin)
