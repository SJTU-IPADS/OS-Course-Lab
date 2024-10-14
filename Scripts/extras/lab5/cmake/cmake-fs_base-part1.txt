# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

add_library(fs_base STATIC fs_page_cache.c fs_page_fault.c fs_wrapper.c.obj
                           fs_wrapper_ops.c.obj fs_vnode.c.obj)
target_include_directories(fs_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
chcore_copy_all_targets_to_ramdisk()
