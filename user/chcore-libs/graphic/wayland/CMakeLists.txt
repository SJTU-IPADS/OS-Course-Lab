# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

add_subdirectory(dep/libffi)
add_subdirectory(dep/xcursor)

add_library(wayland-core SHARED src/connection.c src/wayland-os.c
                                src/wayland-util.c)
chcore_target_include_and_export_directory(wayland-core PUBLIC include)
chcore_target_include_and_export_directory(wayland-core PUBLIC ../include)
target_link_libraries(wayland-core PUBLIC ffi pipe)

add_library(wayland-server SHARED src/wayland-protocol.c src/xdgshell_protocol.c src/wayland-server.c
                                  src/wayland-shm.c src/event-loop.c)
target_link_libraries(wayland-server PUBLIC wayland-core)

add_library(wayland-client SHARED src/wayland-protocol.c src/xdgshell_protocol.c src/wayland-client.c)
target_link_libraries(wayland-client PUBLIC wayland-core)

add_library(wayland-cursor SHARED src/wayland-cursor.c)
target_link_libraries(wayland-cursor PUBLIC wayland-core xcursor)
