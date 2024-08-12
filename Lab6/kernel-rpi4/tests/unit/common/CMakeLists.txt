# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

project(test_common)
set(_kernel_source_dir ../../..)
set(OBJECT_DIR ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/test_common.dir)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -g")


add_executable(test_list test_list.c stub.c)
add_executable(test_hashtable test_hashtable.c stub.c)
add_executable(test_radix test_radix.c stub.c)
add_executable(test_rbtree test_rbtree.c stub.c)

include_directories(${_kernel_source_dir}/include/common
                    ${_kernel_source_dir}/include)

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
    DEPENDS test_list)

enable_testing()
add_test(test_list ${CMAKE_CURRENT_BINARY_DIR}/test_list)
add_test(test_hashtable ${CMAKE_CURRENT_BINARY_DIR}/test_hashtable)
add_test(test_radix ${CMAKE_CURRENT_BINARY_DIR}/test_radix)
add_test(test_rbtree ${CMAKE_CURRENT_BINARY_DIR}/test_rbtree)
