# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

add_subdirectory(libschrift)
add_subdirectory(libtsm)

add_user_executable(terminal.bin main.c terminal.c font.c keymap.c)
chcore_target_remove_link_options(terminal.bin -static)
target_link_libraries(terminal.bin PRIVATE schrift tsm wayland-client)
chcore_copy_all_targets_to_ramdisk()
