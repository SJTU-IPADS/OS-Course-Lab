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
    echo "Usage: $self [[overrides_absolute_path]] [[target_absolute_path]]"
    exit 1
}

if [[ "$#" -ne 2 ]]; then
    usage
fi

if [[ $1 != /* ]] || [[ $2 != /* ]]; then
    usage 
fi

overrides=$1
target=$2

# Overrides is based on mirror structure of $overrides and $target directory.
# So, for each file and directory **under $overrides**, we have defined following
# overriding semantics:
# 1. For a directory, if there is no corresponding directory in $target, the whole
# directory should exist under $target.
# 2. For a directory(A), if there is corresponding directory in $target(B), we should 
# override all subdirectories and files, but should not touch files not appear in
# A but in B.
# 3. For a file(A), if it is an empty file, it means corresponding file in $target(B)
# should be deleted. We first backup B, then delete it.
# 4. For a file(A), if it is not empty, we first backup its corresponding file(B), then
# override B with content of A.
#
# Based on above overridng semantic, this process is implemented with target state-based
# approach. Besides, for performance and being able to reflect latest changes of libchcore,
# We use symlink for all override operations. So, consider the target state of a directory 
# and file under $override:
# 1. For a directory(A), if B exists, then we recurse to check subdirectories and files of A.
# 2. For a directory(A), if B not exists, then the target state should be that B is a 
# symlink to A.
# 3. For a file(A), if it is empty, then the target state should be: (1) backup file of B
# exists, and (2) B is deleted
# 4. For a file(A), if it is not empty, then the target state should be: (1) backup file of B
# exists, and (2) B is a symlink to A
# 
# We recursively iterate over each subdirectories and files under $override, for each of 
# them, check the state of their corresponding B is reached target state or not. If reached,
# we just skip for idempotence. Otherwise, we perform backup and link operations(if needed).

function traverse {
    for file in "$1"/*; do
        rel_path=${file#$overrides/}
        target_file=$target/$rel_path

        if [[ -d "$file" ]]; then
            if [[ ! -e "$target_file" ]]; then
                ln -sf "$file" "$target_file"
                echo "--- Overrided ${target_file} with ${file} symlink"
            elif [[ ! -L "$target_file" ]]; then
                traverse "$file"
            else
                echo "--- Target directory ${target_file} has been overrided, skipping..."
            fi
        elif [[ -f "$file" ]]; then
            if [[ "$file" == *.del ]]; then
                target_file="${target_file%.del}"
                if [[ -e "${target_file}" ]]; then
                    mv "${target_file}" "$target_file.bak"
                    echo "--- Overrided ${target_file} with ${file}"
                else
                    echo "--- Target file ${target_file} has been deleted, skipping..."
                fi
            else
                if [[ ! -e "$target_file" ]]; then
                    ln -sf "$file" "$target_file"
                    echo "--- Overrided ${target_file} with ${file}"
                elif [[ ! -L "$target_file" ]]; then
                    mv "$target_file" "$target_file.bak"
                    ln -sf "$file" "$target_file"
                    echo "--- Overrided ${target_file} with ${file}"
                else
                    echo "--- Target file ${target_file} is already a symlink, skipping..."
                fi
            fi
        fi
    done
}

traverse $overrides
