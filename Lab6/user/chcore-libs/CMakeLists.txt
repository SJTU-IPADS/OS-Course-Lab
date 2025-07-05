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
project(ChCoreLibraries ASM C)

file(WRITE "${CMAKE_BINARY_DIR}/empty.c" "// empty.c")
set(CHCORE_EMPTY_C "${CMAKE_BINARY_DIR}/empty.c")

include(CommonTools)
include(LibAppTools)

add_subdirectory(sys-libs)
# if(CHCORE_LIBRARY_DEVICE)
#     add_subdirectory(dev_sdk)
# endif()
# if(CHCORE_LIBRARY_GRAPHIC)
    add_subdirectory(graphic)
# endif()
add_subdirectory(drivers)

chcore_export_libs_as_package(chcore-libs)
