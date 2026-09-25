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

# Add thirdparty/json/single_include/nlohmann directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add executable
add_executable(json_example json.c)
add_executable(llama_example llama.c)

# Link the simple_json library to the executable
target_link_libraries(json_example PRIVATE simple_json)
target_link_libraries(llama_example PRIVATE ggml simple_llama_chat)
