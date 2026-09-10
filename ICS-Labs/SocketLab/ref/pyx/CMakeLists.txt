#
# Copyright (c) 2025 Institute of Parallel And Distributed Systems (IPADS),
# Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
# use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
# KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
# Mulan PSL v2 for more details.
#

# find python packages here
find_package(Python REQUIRED COMPONENTS Development Interpreter)

# call cython to convert pyserver.pyx to c
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pyserver.c
    COMMAND cython --embed -3 -o ${CMAKE_CURRENT_BINARY_DIR}/pyserver.c
        ${CMAKE_CURRENT_SOURCE_DIR}/pyserver.pyx
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pyserver.pyx
)

add_library(pyserver MODULE ${CMAKE_CURRENT_BINARY_DIR}/pyserver.c)
target_include_directories(pyserver PRIVATE ${Python_INCLUDE_DIRS})
target_link_libraries(pyserver PRIVATE simple_llama_chat ${Python_LIBRARIES})
set_target_properties(pyserver PROPERTIES PREFIX "")

# call cython to convert pyclient.pyx to c
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pyclient.c
    COMMAND cython --embed -3 -o ${CMAKE_CURRENT_BINARY_DIR}/pyclient.c
        ${CMAKE_CURRENT_SOURCE_DIR}/pyclient.pyx
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pyclient.pyx
)

add_library(pyclient MODULE ${CMAKE_CURRENT_BINARY_DIR}/pyclient.c)
target_include_directories(pyclient PRIVATE ${Python_INCLUDE_DIRS})
target_link_libraries(pyclient PRIVATE ${Python_LIBRARIES})
set_target_properties(pyclient PROPERTIES PREFIX "")
