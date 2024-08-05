# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

target_sources(
    ${kernel_target}
    PRIVATE bzero.S.obj
            cache.c.obj
            memcpy.S.obj
            memmove.S.obj
            memset.c.obj
            page_table.c
            page_table.S.obj
            tlb.c.obj
            uaccess.c.obj
            vmspace.c.obj
            copy_from_user.S.obj
            copy_to_user.S.obj)
