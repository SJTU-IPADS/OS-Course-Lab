# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

add_library(libchcore STATIC chcore_mman.h chcore_mmap.c chcore_shm.c chcore_shm.h cpio.c eventfd.c eventfd.h fd.c fd.h file.c file.h fs_client.c fs_client_defs.h futex.c futex.h ipc.c liblauncher.c memory.c pipe.c pipe.h poll.c poll.h rbtree.c rbtree_plus.c ring_buffer.c services.c socket.c socket.h stdio.c syscall.c syscall_dispatcher.c timerfd.c timerfd.h syscall_get_system_info.c)