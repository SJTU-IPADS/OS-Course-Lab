#!/usr/bin/env bash
# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

set -e

self=`basename "$0"`

function usage {
    echo "Usage: $self [patches_absolute_path] [target_absolute_path]"
    exit 1
}

if [ "$#" -ne 2 ]; then
    usage
fi

if [[ $1 != /* ]] || [[ $2 != /* ]]; then
    usage 
fi

pacthes=$1
target=$2

# Patching is based on mirror structure of $pacthes and $target directory.
# So, for each file **under $pacthes**, we have defined following
# patching semantics:
# If this file is a *.sh or *.patch file, it indicates that this file is a patch file,
# targeting corresponding files under $target without .sh or .patch.
#
# Based on above patching semantic, this process is implemented with target state-based
# approach. The target state of each file to be patched under $target is that their 
# backups have been created, and their content have been patched using patch files.
# 
# We recursively iterate over each subdirectories and files under $pacthes, for each patch 
# file, check the state of their corresponding B is reached target state or not. If reached,
# we just skip for idempotence. Otherwise, we perform backup and use patch file to patch
# target file(by using patch command or executing .sh).

function traverse {
    for file in "$1"/*; do
        rel_path=${file#$pacthes/}
        target_file=$target/${rel_path%.*}
        bak_file=$target_file.bak

        if [ -d "$file" ]; then
            traverse "$file"
        elif [ -f "$file" ]; then
            ext=${file##*.}
            if [ "$ext" = "patch" ] || [ "$ext" = "sh" ]; then
                if [ ! -e "$target_file" ] || [ -e $bak_file ]; then
                    continue
                else
                    cp "$target_file" "$target_file.bak"
                fi

                if [ "$ext" = "patch" ]; then
                    patch "$target_file" "$file"
                    echo "--- Patched ${target_file} with ${file}"
                elif [ "$ext" = "sh" ]; then
                    bash "$file" "$target_file"
                    echo "--- Patched ${target_file} with ${file}"
                fi
            fi
        fi
    done
}

traverse $pacthes