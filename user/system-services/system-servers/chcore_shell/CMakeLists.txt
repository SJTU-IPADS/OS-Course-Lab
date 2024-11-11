# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

chcore_enable_clang_tidy()
if(CHCORE_MINI)
    add_user_executable_static_linked(chcore_shell.bin chcore_shell.c job_control.c buildin_cmd.c
                                    handle_input.c)
else()
    add_user_executable(chcore_shell.bin chcore_shell.c job_control.c buildin_cmd.c
                                handle_input.c)
endif()

chcore_copy_all_targets_to_ramdisk()
