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

project(test_buddy_and_slab C)
set(_kernel_source_dir ../../..)
set(SOURCE_PATH ${_kernel_source_dir}/mm)
set(OBJECT_DIR ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_buddy_and_slab.dir)

# Pass defined macros here
set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -g -DON=1 -DGET_CHUNK_NUM_IN_BUDDY"
)

set(ARCH "aarch64")
set(PLAT "raspi3")

set(ALLOCATOR_SOURCES test_buddy_and_slab.c stub.c "${SOURCE_PATH}/buddy.c"
                      "${SOURCE_PATH}/slab.c" "${SOURCE_PATH}/kmalloc.c")

set(MULTI_BUDDY_SOURCES stub.c test_multi_buddy.c "${SOURCE_PATH}/buddy.c"
                        "${SOURCE_PATH}/slab.c" "${SOURCE_PATH}/kmalloc.c")

set(VMR_SPLIT_SOURCES stub.c test_vmr_split.c "${SOURCE_PATH}/../lib/rbtree.c")

add_executable(test_buddy_and_slab ${ALLOCATOR_SOURCES})
add_executable(test_multi_buddy ${MULTI_BUDDY_SOURCES})
add_executable(test_vmr_split ${VMR_SPLIT_SOURCES})

include_directories(${_kernel_source_dir}/include/
                    ${_kernel_source_dir}/include/arch/${ARCH}/)

target_compile_options(test_buddy_and_slab PRIVATE -fno-builtin-memset
                                                   -fno-builtin-memcpy)

target_compile_options(test_multi_buddy PRIVATE -fno-builtin-memset
                                                -fno-builtin-memcpy)

target_compile_options(test_vmr_split PRIVATE -fno-builtin-memset
                                              -fno-builtin-memcpy)

target_link_libraries(test_buddy_and_slab -lm)

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
    DEPENDS test_buddy_and_slab)

enable_testing()
add_test(test_buddy_and_slab test_buddy_and_slab)
add_test(test_multi_buddy test_multi_buddy)
add_test(test_vmr_split test_vmr_split)
