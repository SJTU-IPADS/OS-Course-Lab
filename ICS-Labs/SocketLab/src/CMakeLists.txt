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

# Add executable for the server
add_executable(server server.c)
target_link_libraries(server PRIVATE simple_json simple_llama_chat)

# Add executable for the server_epoll
add_executable(server_epoll server_epoll.c)
target_link_libraries(server_epoll PRIVATE simple_json simple_llama_chat)

# Add executable for the client
add_executable(client client.c)
target_link_libraries(client PRIVATE simple_json simple_llama_chat)
