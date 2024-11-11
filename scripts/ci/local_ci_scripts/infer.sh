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

infer capture --compilation-database kernel/build/compile_commands.json
infer analyze >kernel_fbinfer.res 2>&1
cat kernel_fbinfer.res
grep "No issues found" kernel_fbinfer.res
ret_kern=$?
cp infer-out/report.txt infer_kernel_report.txt

rm -rf kernel/build
rm -rf infer-out kernel_fbinfer.res

exit $ret_kern
