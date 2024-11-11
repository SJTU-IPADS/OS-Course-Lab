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

project(test_x64_page_table C)
set(_kernel_source_dir ../../../..)
set(SOURCE_PATH ${_kernel_source_dir}/arch/x86_64)
set(OBJECT_DIR ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_x64_page_table.dir)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -g")

set(SOURCES test_x64_page_table.c)

add_executable(test_x64_page_table ${SOURCES})
include_directories(${_kernel_source_dir}/include/arch/x86_64/
                    ${_kernel_source_dir}/include/)

add_custom_target(
    lcov-${PROJECT_NAME}
    COMMAND lcov -d ${CMAKE_CURRENT_SOURCE_DIR} -z
    COMMAND lcov -d ${CMAKE_CURRENT_SOURCE_DIR} -b . --initial -c -o lcov.info
    COMMAND CTEST_OUTPUT_ON_FAILURE=1 ${CMAKE_MAKE_PROGRAM} test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(
    TARGET lcov-${PROJECT_NAME}
    COMMAND lcov -d ${CMAKE_CURRENT_SOURCE_DIR} -c -o lcov.info
    COMMAND genhtml -o report --prefix=`pwd` lcov.info
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS test_x64_page_table)

enable_testing()
add_test(test_x64_page_table test_x64_page_table)
