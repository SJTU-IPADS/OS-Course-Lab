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

project(test_object C)
set(_kernel_source_dir ../../..)
set(SOURCE_PATH ${_kernel_source_dir}/object)
set(OBJECT_DIR ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_object.dir)
set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -pthread -g -DTEST_OBJECT")

if(ARCH)

else()
    set(ARCH "aarch64")
endif()
if(PLAT)

else()
    set(PLAT "raspi3")
endif()

file(GLOB KERN_SOURCE "${SOURCE_PATH}/thread.c" "${SOURCE_PATH}/cap_group.c"
        "${SOURCE_PATH}/capability.c" "${SOURCE_PATH}/../syscall/syscall_hooks.c")
set(SOURCES ${KERN_SOURCE} test_capability.c test_object.c stub.c)

add_executable(test_object ${SOURCES})
include_directories(
    ./include ${_kernel_source_dir}/include
    ${_kernel_source_dir}/include/arch/${ARCH}
    ${_kernel_source_dir}/include/arch/${ARCH}/plat/${PLAT})

target_compile_options(
    test_object PRIVATE -fno-builtin-memset -fno-builtin-memcpy
                        -fno-builtin-memmove)

add_custom_target(
    lcov-${PROJECT_NAME}
    COMMAND lcov -d ${CMAKE_CURRENT_SOURCE_DIR} -z
    COMMAND lcov -d ${CMAKE_CURRENT_SOURCE_DIR} -b . --initial -c -o lcov.info
    COMMAND ${CMAKE_MAKE_PROGRAM} CTEST_OUTPUT_ON_FAILURE=1 test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(
    TARGET lcov-${PROJECT_NAME}
    COMMAND lcov -d ${CMAKE_CURRENT_SOURCE_DIR} -c -o lcov.info
    COMMAND genhtml -o report --prefix=`pwd` lcov.info
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS test_object)

enable_testing()
add_test(test_object test_object)
