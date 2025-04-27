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

add_library(simple_llama_chat simple_llama_chat.cpp)
target_link_libraries(simple_llama_chat PRIVATE llama ${CMAKE_THREAD_LIBS_INIT})
target_compile_features(simple_llama_chat PRIVATE cxx_std_17)
target_include_directories(simple_llama_chat PUBLIC
    ${CMAKE_SOURCE_DIR}/thirdparty/llama.cpp/include
    ${CMAKE_SOURCE_DIR}/include)
