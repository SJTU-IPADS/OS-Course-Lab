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

cmake_minimum_required(VERSION 3.20)

project("ICS-Socket-Lab" VERSION 1.0.0 LANGUAGES C CXX)

# Output compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compile position independent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add thirdparty/json/single_include/nlohmann directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/json/single_include/nlohmann)

# Add subdirectories
add_subdirectory(examples)
add_subdirectory(lib)
add_subdirectory(src)

# Build sub-project llama.cpp in thirdparty/llama.cpp
# Set static linking for llama.cpp
set(LLAMA_BUILD_COMMON OFF)
set(LLAMA_CURL OFF)
set(LLAMA_BUILD_EXAMPLES OFF)
set(LLAMA_BUILD_TESTS OFF)
# set(BUILD_SHARED_LIBS OFF)
add_subdirectory(thirdparty/llama.cpp)
