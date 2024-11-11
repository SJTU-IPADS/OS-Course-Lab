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

# Remove log
sudo rm -f ./exec_log

# Killall screen
# sudo killall screen

# Execute expect script
# Note: $2 is only used as device num for HiKey970
echo $1 $2
$1 $2

ret=$?
echo "The exit value of CI script: $ret"

# Check return value
if [ $ret -eq 0 ]; then
        cat ./exec_log
        echo ""
        echo "Succeeded to run expect script $1"
elif [ $ret -eq 1 ]; then
        cat ./exec_log
        echo ""
        echo "[CI] Run expect script $1 timeout, exit value: $ret"
        exit $ret
else
        cat ./exec_log
        echo ""
        echo "[CI] Failed to run expect script $1, exit value: $ret"
        exit $ret
fi
