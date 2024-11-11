#!/bin/bash
# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

# For board related screens cleaning (they must be run as root to access /dev/ttyUSBX)
sudo screen -ls | grep -E "Detached|Dead" | cut -d. -f1 | awk '{print $1}' | sudo xargs kill
# For other screen sessions running as gitlab-runner
screen -ls | grep -E "Detached|Dead" | cut -d. -f1 | awk '{print $1}' | xargs kill
