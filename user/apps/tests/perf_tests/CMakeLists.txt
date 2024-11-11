# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

add_subdirectory(perf_tools)

macro(add_perf_executable _target)
    add_user_executable(${_target} ${ARGN})
    target_link_libraries(${_target} PRIVATE perf_tools)
endmacro(add_perf_executable)

add_perf_executable(perf_syscall.bin perf_syscall.c)
add_perf_executable(perf_cs.bin perf_cs.c)
add_perf_executable(perf_ipc.bin perf_ipc.c)
add_perf_executable(perf_ipc_server.bin perf_ipc_server.c)
add_perf_executable(perf_pgfault.bin perf_pgfault.c)
add_perf_executable(perf_fs.bin perf_fs.c)
add_perf_executable(perf_create_process.bin perf_create_process.c)
add_perf_executable(perf_pthread.bin perf_pthread.c)
add_perf_executable(perf_process_switch.bin perf_process_switch.c)
add_perf_executable(perf_second_process.bin perf_second_process.c)
add_perf_executable(perf_notific.bin perf_notific.c)
add_perf_executable(perf_semphore.bin perf_semphore.c)
add_user_executable(blank.bin blank.c)
