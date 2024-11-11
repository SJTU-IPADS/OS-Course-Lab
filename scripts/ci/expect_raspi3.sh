#!/bin/sh
# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

{
    flock -n 9
    if [ $? -ne 0 ]; then
        echo "board is in use, exit!"
        exit 1
    fi

    scp kernel/arch/aarch64/boot/raspi3/firmware/* pi@chos-pi-ci:/home/pi/tftpboot
    scp build/kernel8.img pi@chos-pi-ci:/home/pi/tftpboot
    ssh pi@chos-pi-ci "./reset_ch1.py"
    ./scripts/ci/expect_wrapper.sh $1
    res=$?
    flock -u 9
    if [ ${res} -ne 0 ]; then
        exit ${res}
    fi
} 9<>./scripts/ci/raspi.lock
