# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

target_sources(${kernel_target} PRIVATE page_table.c)
chcore_target_precompile(
    ${kernel_target}
    PRIVATE bzero.S
            cache.c
            memcpy.S
            memmove.S
            memset.c
            page_table.S
            tlb.c
            uaccess.c
            vmspace.c
            copy_from_user.S
            copy_to_user.S)
