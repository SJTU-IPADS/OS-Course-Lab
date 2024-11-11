# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

add_subdirectory(test_tools)

macro(add_test_executable _target)
    add_user_executable(${_target} ${ARGN})
    target_link_libraries(${_target} PRIVATE test_tools)
endmacro(add_test_executable)

macro(add_test_executable_static _target)
    add_user_executable_static(${_target} ${ARGN})
    target_link_libraries(${_target} PRIVATE test_tools)
endmacro(add_test_executable_static)

add_test_executable(test_ipc.bin test_ipc.c)
add_test_executable(test_ipc_perf.bin test_ipc_perf.c)
add_test_executable(test_ipc_server.bin test_ipc_server.c)

add_test_executable(test_sched.bin test_sched.c)

add_test_executable(test_net.bin test_net.c)
add_test_executable(test_net_server.bin test_net_server.c)
add_test_executable(test_net_client.bin test_net_client.c)
add_test_executable(test_udp_server.bin test_udp_server.c)
add_test_executable(test_udp_client.bin test_udp_client.c)

add_test_executable(test_poll_net.bin test_poll_net.c)
add_test_executable(test_epoll_net.bin test_epoll_net.c)
add_test_executable(poll_client.bin poll_client.c)
add_test_executable(poll_server.bin poll_server.c)
add_test_executable(epoll_server.bin poll_server.c)
target_compile_options(epoll_server.bin PRIVATE -DUSE_EPOLL)

add_test_executable(test_malloc.bin test_malloc.c)

add_test_executable(test_fs.bin test_fs.c)
target_compile_options(test_fs.bin PRIVATE -g3 -ggdb)
target_include_directories(test_fs.bin PRIVATE ../../chcore-libs/sys-libs/libboundscheck/include)
target_link_libraries(test_fs.bin PRIVATE boundscheck)

add_test_executable(test_fpu.bin test_fpu.c)

add_test_executable(test_notifc.bin test_notifc.c)

add_test_executable(test_pthread.bin test_pthread.c)
add_test_executable(test_mutex_ceiling.bin test_mutex_ceiling.c)

add_test_executable(test_time.bin test_time.c)

add_test_executable(test_eventfd.bin test_eventfd.c)
add_test_executable(test_timerfd.bin test_timerfd.c)

add_test_executable(test_poll_efd_tfd.bin test_poll_efd_tfd.c)
add_test_executable(test_epoll_efd_tfd.bin test_epoll_efd_tfd.c)

add_test_executable(test_sleep.bin test_sleep.c)

add_test_executable(test_all.bin test_all.c)

add_test_executable(test_clockgettime.bin test_clockgettime.c)

add_test_executable(test_long_term_running.bin test_long_term_running.c)


add_test_executable(test_fork_loop.bin test_fork_loop.c)
add_test_executable(test_hello.bin test_hello.c)

add_test_executable(test_syscall64.bin test_syscall64.c)

add_test_executable(test_mm_leakage.bin test_mm_leakage.c)

add_test_executable(test_scanf.bin test_scanf.c)

add_test_executable(test_div_zero.bin test_div_zero.c)

add_test_executable(test_vaddr.bin test_vaddr.c)

add_test_executable(test_mprotect.bin test_mprotect.c)

add_test_executable(test_unmap_multiple_vmrs.bin test_unmap_multiple_vmrs.c)

add_test_executable(test_orphan.bin test_orphan.c)
if(CHCORE_DRIVER_AUDIO)
    add_test_executable(test_audio.bin test_audio.c)
    target_link_libraries(test_audio.bin PRIVATE audio_driver)
endif()
add_test_executable(test_access_libc_pmo.bin test_access_libc_pmo.c)
add_test_executable(test_revoke_all_libc_pmo.bin test_revoke_all_libc_pmo.c)
add_test_executable(test_funmap.bin test_funmap.c)

if(CHCORE_SERVER_POSIX_SHM)
    add_test_executable(test_shm_client.bin test_shm_client.c)
    add_test_executable(test_shm_server.bin test_shm_server.c)
    add_test_executable(test_shm_multi.bin test_shm_multi.c)
endif()

# Filesystem tests
add_subdirectory(fs_tests)
# Performance tests
add_subdirectory(perf_tests)

# Realtime tests
if(CHCORE_KERNEL_RT)
    add_subdirectory(rt_tests)
endif()

# Open Harmony TEE tests
if(CHCORE_OH_TEE)
    add_subdirectory(oh_tee_ipc)
endif()

if(CHCORE_ARCH STREQUAL "sparc")
    add_subdirectory(vx_tests)
endif()

if(CHCORE_SERVER_GDB)
    add_test_executable_static(test_gdb.bin test_gdb.c)
    chcore_target_force_static_linked(test_gdb.bin)
endif()

add_subdirectory(api_tests)

chcore_all_dynamic_linked_exec()

chcore_copy_all_targets_to_ramdisk()

chcore_copy_files_to_ramdisk(test.txt)
