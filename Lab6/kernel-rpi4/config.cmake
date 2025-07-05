# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

chcore_config(CHCORE_KERNEL_DEBUG BOOL OFF "Build debug version of the kernel?")
chcore_config(CHCORE_KERNEL_TEST BOOL OFF "Enable kernel tests?")
chcore_config(CHCORE_KERNEL_RT BOOL OFF "Enable realtime support in kernel?")
chcore_config(CHCORE_KERNEL_SCHED_PBFIFO BOOL OFF "Use priority-based FIFO?")
chcore_config(CHCORE_KERNEL_ENABLE_QEMU_VIRTIO_NET BOOL ON "Enable virtio-net nic on x86_64 QEMU?")
chcore_config(CHCORE_KERNEL_DISALLOW_ACCESS_PHYMEM BOOL ON "limit the creation of pmo object for PMO_DEVICE type and get phy addr?")
