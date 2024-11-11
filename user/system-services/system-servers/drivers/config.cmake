# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

chcore_config(CHCORE_DRIVER_FWK_LINUX BOOL OFF "Build Linux driver framework (linux-port)?")
chcore_config(CHCORE_DRIVER_FWK_CIRCLE BOOL OFF "Build Circle driver framework?")

#[[
The following configs are common to all architectures and platforms, while
different architectures and platforms may build different implementations
of these drivers. For example, on Raspberry Pi platform, Circle framework
is used for CHCORE_DRIVER_ETHERNET, while a dedicated hdmi driver is used
for CHCORE_DRIVER_HDMI.

So the following configs can be viewed as "whether to enable the feature",
while which driver implementation to use is controlled by the CMakeLists of
each architecture or platform.
#]]
chcore_config(CHCORE_DRIVER_USB BOOL ON "Build USB driver?")
chcore_config(CHCORE_DRIVER_HDMI BOOL ON "Build HDMI driver?")
chcore_config(CHCORE_DRIVER_ETHERNET BOOL ON "Build ethernet driver?")
chcore_config(CHCORE_DRIVER_CPU_THROTTLE BOOL ON "Build CPU throttle driver?")
chcore_config(CHCORE_DRIVER_GPIO BOOL ON "Build GPIO driver?")
chcore_config(CHCORE_DRIVER_SD BOOL ON "Build SD driver?")
chcore_config(CHCORE_DRIVER_SERIAL BOOL ON "Build serial port driver?")
chcore_config(CHCORE_DRIVER_WLAN BOOL ON "Build WLAN driver?")
chcore_config(CHCORE_DRIVER_FLASH BOOL ON "Build flash driver?")
chcore_config(CHCORE_DRIVER_AUDIO BOOL ON "Build audio driver?")
