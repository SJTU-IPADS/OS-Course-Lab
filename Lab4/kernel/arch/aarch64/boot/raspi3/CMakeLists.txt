# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

target_include_directories(${kernel_target} PRIVATE include)

list(
    APPEND
    _init_sources
    init/start.S
    init/mmu.c
    init/tools.S
    init/init_c.c
    peripherals/uart.c)

chcore_target_precompile_out_objects(${kernel_target} PRIVATE init_objects
                                  ${_init_sources})

set(init_objects
    ${init_objects}
    PARENT_SCOPE)

chcore_objcopy_binary(${kernel_target} kernel8.img)
