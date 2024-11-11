# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LWIP_INCLUDE_DIRS ports/chos/include src/include src/include/lwip)
set(LWIP_COMPILER_FLAGS -O3)
set(LWIP_DEFINITIONS -D_GNU_SOURCE)

include(src/Filelists.cmake)

# Disable warnings for LWIP core source files
chcore_target_remove_compile_options(lwipcore -Wall -Werror)
target_compile_options(lwipcore PRIVATE -w)

set(_chcore_port_sources ports/chos/sys_arch.c ports/chos/debug_flags.c)

if(!CHCORE_MINI)
	add_user_executable(lwip.srv ${_chcore_port_sources} "main.c" "interface.c")
else()
	add_user_executable_static_linked(lwip.srv ${_chcore_port_sources} "main.c" "interface.c")
endif()

target_include_directories(lwip.srv PRIVATE ${LWIP_INCLUDE_DIRS})
target_compile_options(lwip.srv PRIVATE ${LWIP_COMPILER_FLAGS})
target_compile_definitions(lwip.srv PRIVATE ${LWIP_DEFINITIONS})
target_link_libraries(lwip.srv PRIVATE lwipcore)

if(!CHCORE_MINI)
	add_user_executable(tcp_example.bin ${_chcore_port_sources} "tcpip_example.c")
	target_include_directories(tcp_example.bin PRIVATE ${LWIP_INCLUDE_DIRS})
	target_compile_options(tcp_example.bin PRIVATE ${LWIP_COMPILER_FLAGS})
	target_compile_definitions(tcp_example.bin PRIVATE ${LWIP_DEFINITIONS})
	target_link_libraries(tcp_example.bin PRIVATE lwipcore pthread)
endif()
chcore_copy_all_targets_to_ramdisk()
