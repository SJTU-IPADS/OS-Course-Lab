# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

add_user_executable(client.bin client.c)
add_user_executable(server.bin server.c)

add_user_executable(hello_world.bin hello_world.c)

add_user_executable(cat.bin cat.c)

add_user_executable(diff.bin diff.c)

add_user_executable(mount.bin mount.c)

add_user_executable(pmu.bin pmu.c)

add_user_executable(get_system_info.bin get_system_info.c)

if(CHCORE_APP_SAMPLE_CXX)
        add_subdirectory(cxx)
endif()

add_subdirectory(dynamic_link)

add_executable(static_sample.bin static_sample.c)

add_subdirectory(simple_http_server)
add_user_executable(udp_echo_server.bin udp_echo_server.c)
add_user_executable(tcp_echo_server.bin tcp_echo_server.c)
add_user_executable(pc pc.c)

chcore_all_dynamic_linked_exec()

chcore_target_force_static_linked(static_sample.bin)

chcore_copy_all_targets_to_ramdisk()