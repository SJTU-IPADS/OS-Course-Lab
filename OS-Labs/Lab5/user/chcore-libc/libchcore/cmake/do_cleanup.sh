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

self=$(basename "$0")

function usage {
    echo "Usage: $self [target_absolute_path]"
    exit 1
}

if [ "$#" -ne 1 ]; then
    usage
fi

if [[ $1 != /* ]]; then
    usage
fi

target=$1

# Cleaning up is based on mirror structure of the original backups.
# We recursively iterate over each subdirectories and files under $target, for each file, check whether it's a symlink. If it's a symlink, then delete it and check whether its backup file exists.
# For remaining backup files, we recover it without cleaning up the symlink.

function traverse {
    for file in "$1"/*; do
        if [[ -d "$file" ]]; then
            if [[ -f "$file.sym" ]]; then
                rm -rf "$file"
                rm "$file.sym"
                echo "--- Cleanup ${file}"
            else
                traverse "$file"
            fi
        elif [[ -f "$file" && "$file" != *.bak ]]; then
            if [[ -f "$file.sym" ]]; then
                rm "$file.sym"
                rm "$file"
                echo "--- Cleanup ${file}"
            fi
            if [[ -f "$file.bak" ]]; then
                mv "$file.bak" "$file"
                echo "--- Recover ${file}"
            fi
        elif [[ -f "$file" && "$file" == *.bak ]]; then
            mv $file ${file%.bak}
            echo "--- Recover ${file}"
        fi
    done
}

traverse $target
