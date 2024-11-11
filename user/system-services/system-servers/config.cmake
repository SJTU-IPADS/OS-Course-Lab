# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

chcore_config(CHCORE_SERVER_GUI BOOL OFF "Build GUI server?")
chcore_config(CHCORE_SERVER_POSIX_SHM BOOL ON "Build POSIX shmxx API server?")
chcore_config(CHCORE_SERVER_GDB BOOL OFF "Build GDB server?")
chcore_config(CHCORE_SERVER_LWIP BOOL ON "Build the lwip server?")

chcore_config(CHCORE_FS_FAT32 BOOL ON "Build FAT32 filesystem server?")
chcore_config(CHCORE_FS_EXT4 BOOL ON "Build ext4 filesystem server?")
chcore_config(CHCORE_FS_LITTLEFS BOOL ON "Build LittleFS filesystem server?")
chcore_config(CHCORE_FS_LITTLEFS_BD STRING "ram" "Use which block device config for LittleFS? (ram|sstflash)")

chcore_config(CHCORE_DAEMON_NETCP BOOL ON "Build the Network-CP daemon?")

chcore_config(CHCORE_STATIC_IP BOOL OFF "Using a static IP address instead of DHCP?")

chcore_config(CHCORE_APP_SHELL BOOL ON "Build the chcore-shell server?")

chcore_config_include(drivers/config.cmake)
