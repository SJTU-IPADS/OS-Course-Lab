# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

cmake_minimum_required(VERSION 3.1)

project(
  ramdisk-rebuild-sample
  LANGUAGES C CXX)

add_executable(hello_chcore hello_chcore.c)

add_executable(hello_chcore_cpp hello_chcore_cpp.cpp)

install(TARGETS hello_chcore hello_chcore_cpp DESTINATION ${CHCORE_RAMDISK_INSTALL_PREFIX})