# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

file(GLOB _fs_test_sources "fs_tools/*.c")

add_executable(fs_test_full.bin fs_test_full.c ${_fs_test_sources})
target_include_directories(fs_test_full.bin PRIVATE fs_tools)
target_link_libraries(fs_test_full.bin PRIVATE pthread)

add_executable(fs_test_mmap.bin fs_test_mmap.c ${_fs_test_sources})
target_include_directories(fs_test_mmap.bin PRIVATE fs_tools)
target_link_libraries(fs_test_mmap.bin PRIVATE pthread)
