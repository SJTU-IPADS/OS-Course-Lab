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

project(test_common C CXX)
set(OBJECT_DIR ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_common.dir)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -g")

add_executable(test_user_radix test_user_radix.c)
add_executable(test_user_ringbuffer test_user_ringbuffer.c ring_buffer.c)
add_executable(test_rbtree_plus test_rbtree_plus.c)
add_executable(test_user_rbtree test_rbtree.c)
add_executable(test_user_vaddr_alloc test_vaddr_alloc.c)

target_link_libraries(test_user_ringbuffer pthread)
target_link_libraries(test_user_vaddr_alloc pthread)

add_custom_target(
    lcov
    COMMAND lcov -d ${CMAKE_CURRENT_SOURCE_DIR} -z
    COMMAND lcov -d ${CMAKE_CURRENT_SOURCE_DIR} -b . --initial -c -o lcov.info
    COMMAND CTEST_OUTPUT_ON_FAILURE=1 ${CMAKE_MAKE_PROGRAM} test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
add_custom_command(
    TARGET lcov
    COMMAND lcov -d ${CMAKE_CURRENT_SOURCE_DIR} -c -o lcov.info
    COMMAND genhtml -o report --prefix=`pwd` lcov.info
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS test_user_radix test_rbtree test_user_ringbuffer
)

enable_testing()
add_test(test_user_radix test_user_radix)
add_test(test_user_ringbuffer test_user_ringbuffer)
add_test(test_rbtree_plus test_rbtree_plus)
add_test(test_user_rbtree test_user_rbtree)
add_test(test_user_vaddr_alloc test_user_vaddr_alloc)
