#!/usr/bin/env python3
# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

# XXX: This script is temporarily used to wipe off "\n$ " pattern in output of tests in expect scripts.
# "\n$ " is the prefix printed in init.bin, which can interfere with app's output.

# Why not use existing tools (sed, tr, etc.)? Because 1. some tools process text at
# line granularity thus is improper to handle '\n' and 2. some tools buffer texts
# internally so that inconvenient to handle streaming outputs.

import sys

str = "\n$ "
idx = 0

for line in sys.stdin:
    for char in line:
        if char == str[idx]:
            if idx == len(str) - 1:
                idx = 0
            else:
                idx += 1
        else:
            for i in range(0, idx):
                print(str[i], end='')
            idx = 0
            print(char, end='')

