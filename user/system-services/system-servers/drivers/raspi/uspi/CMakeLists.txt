# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

# A simple choice:
# Set the binary start address of USPI to 4G.
# Thus, set one-to-one mapping for accessing the device memory (0-4G) on raspi.
set(_cflags
    "-Wno-psabi -fsigned-char -fno-builtin -std=gnu99 \
    -DAARCH64=1 -DRASPPI=3 -Wl,-Ttext-segment=0x100000000 -Wno-unused-variable -Wno-unused-but-set-variable"
)

set(CMAKE_C_FLAGS "${_cflags} ${CMAKE_C_FLAGS}")

file(
    GLOB
    _sources
    main.c
    ethernet.c
    carboard_controller.c
    dispatch/*.c
    lib/*.c
    env/*.c)

add_user_executable_static_linked(uspi.srv ${_sources})
target_include_directories(uspi.srv PRIVATE include env/include
                                            dispatch/include)
target_link_libraries(uspi.srv PRIVATE pipe)

chcore_all_force_static_linked()
chcore_add_configurable_server(server_config.yaml)
