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

CHCORE_PROJECT_DIR=$(pwd)

rm -f infer_kernel_report.txt

rm -rf kernel/build
cmake -S kernel -B kernel/build \
        -DCMAKE_MODULE_PATH=$CHCORE_PROJECT_DIR/scripts/build/cmake/Modules \
        -DCMAKE_TOOLCHAIN_FILE=$CHCORE_PROJECT_DIR/scripts/build/cmake/Toolchains/kernel_fbinfer.cmake \
        -DCHCORE_PROJECT_DIR=$CHCORE_PROJECT_DIR \
        -DCHCORE_USER_INSTALL_DIR= \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=1
