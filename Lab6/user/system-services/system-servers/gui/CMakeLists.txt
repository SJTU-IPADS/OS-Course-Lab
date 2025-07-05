# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

if(CHCORE_PLAT MATCHES "^raspi3")
    add_definitions(-DCHCORE_PLAT_RASPI3)
elseif(CHCORE_PLAT MATCHES "^raspi4")
    add_definitions(-DCHCORE_PLAT_RASPI4)
endif()
file(GLOB gui_srv_src "*.c")
add_subdirectory(guilog)
add_executable(gui.srv ${gui_srv_src})
chcore_target_remove_link_options(gui.srv -static)
target_link_libraries(gui.srv PRIVATE graphic uiutil wayland-server guilog)
target_compile_options(gui.srv PRIVATE -Wno-error)
chcore_copy_all_targets_to_ramdisk()