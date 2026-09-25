# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

chcore_config(CHCORE_CROSS_COMPILE STRING "" "Prefix for cross compiling toolchain")
chcore_config(CHCORE_PLAT STRING "" "Target hardware platform")
chcore_config(CHCORE_SUBPLAT STRING "" "Target hardware platform subtype")
chcore_config(CHCORE_ASLR BOOL ON "Enable ASLR support in ChCore?")
chcore_config(CHCORE_CHPM_INSTALL_PREFIX PATH ".chpm/install" "Install prefix of ChPM (for using packages installed by ChPM)")
chcore_config(CHCORE_CHPM_INSTALL_TO_RAMDISK BOOL OFF "Copy installed ChPM packages to ramdisk?")
chcore_config(CHCORE_CPP_INSTALL_PREFIX PATH ".cpp" "Install prefix of cpp suport (for using libstdc++ package)")
chcore_config(CHCORE_VERBOSE_BUILD BOOL OFF "Generate verbose build log?")
chcore_config(CHCORE_QEMU_SDCARD_IMG PATH "" "Path to SD card image file for QEMU (raspi3)")
chcore_config(CHCORE_USER_DEBUG BOOL OFF "Build debug version of user-level libs and apps?")
chcore_config(CHCORE_QEMU BOOL OFF "Run in QEMU?")
chcore_config(CHCORE_MINI BOOL OFF "Build chcore as small as possible?")

chcore_config_include(kernel/config.cmake)
# chcore_config_include(user/apps/config.cmake)
# chcore_config_include(user/chcore-libs/config.cmake)
# chcore_config_include(user/system-services/config.cmake)
