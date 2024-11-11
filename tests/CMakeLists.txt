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
project(ChCoreUnitTests)

set(_cmake_script_dir ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/build/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${_cmake_script_dir}/Modules)
set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../kernel)

include(CommonTools)

# common include directory for all tests
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../kernel/user-include)

chcore_collect_pkg_path(${CMAKE_CURRENT_SOURCE_DIR}/../user)

enable_testing()
find_package(chcore-libc-unit-tests REQUIRED)
find_package(kernel-unit-tests REQUIRED)
find_package(chcore-tmpfs-unit-tests REQUIRED)
