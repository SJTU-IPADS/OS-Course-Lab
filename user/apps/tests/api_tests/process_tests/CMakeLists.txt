# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

add_user_executable(chcore_new_process.bin test_chcore_new_process.c)
add_user_executable(create_process.bin test_create_process.c)
add_user_executable(create_traced_process.bin test_create_traced_process.c)
add_user_executable(shell_new_proc.bin test_shell_new_proc.c)
add_user_executable(chcore_pthread_create.bin test_chcore_pthread_create.c)