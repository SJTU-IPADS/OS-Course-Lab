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
project(ChCoreApps ASM C CXX)

find_package(chcore-libs REQUIRED)

macro(add_user_executable _target)
    add_executable(${_target} ${ARGN})
    target_link_libraries(${_target} PRIVATE boundscheck)
    target_include_directories(${_target} PRIVATE ../chcore-libs/sys-libs/libboundscheck/include)
endmacro(add_user_executable)

macro(add_user_executable_static _target)
    add_executable(${_target} ${ARGN})
    target_link_libraries(${_target} PRIVATE boundscheck_static)
    target_include_directories(${_target} PRIVATE ../chcore-libs/sys-libs/libboundscheck/include)
endmacro(add_user_executable_static)


include(CommonTools)
include(LibAppTools)

chcore_dump_cmake_vars()

add_compile_options(-Wall)
add_compile_options(-Werror)
string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE
               "${CMAKE_CXX_FLAGS_RELEASE}")

if(CHCORE_APP_DEMO)
        add_subdirectory(demos)
endif()

if(CHCORE_APP_SAMPLE)
        add_subdirectory(samples)
endif()

if(CHCORE_APP_TEST)
        add_subdirectory(tests)
endif()