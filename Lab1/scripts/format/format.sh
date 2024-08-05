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

set -e

script_dir=$(dirname "$0")

RED='\033[0;31m'
BLUE='\033[0;34m'
GREEN='\033[0;32m'
ORANGE='\033[0;33m'
BOLD='\033[1m'
NONE='\033[0m'
C_SOURCES=".*\.(c|h)$"
CPP_SOURCES=".*\.(cpp|cc|hpp|cxx)$"

str=""
cnt=0

main() {
    for mod in "$@"; do
        if [ $cnt -eq 1 ]; then
            let cnt=$cnt+1
            str=""$mod;
        elif [ $cnt -gt 1 ]; then
            str=$str"\|"$mod;
        fi
        if [ $mod == "-exclude" ]; then
            let cnt=$cnt+1
        fi
    done
    for mod in "$@"; do
        if [ $mod == "-exclude" ]; then
            break
        fi
        if [ $cnt -gt 0 ]; then
            for file in $(find $mod -type f | grep -v $str); do
                format $file
            done
        else
            for file in $(find $mod -type f); do
                format $file
            done
        fi
    done
    echo "============"
    echo "Done"
}

format() {
    type=$(file $1)
    if [[ $1 =~ $C_SOURCES || $1 =~ $CPP_SOURCES ]]; then
        echo "Formatting C or C++ file \"$file\"..." 
        clang-format -i -style=file $1
        clang-format -i -style=file $1 # run clang-format twice in case of a bug
    elif (echo $type | grep -q "Bourne-Again shell script"); then
        echo "Formatting Bash script \"$file\"..." 
        shfmt -i 4 -w $1
    elif (echo $type | grep -q "Python script"); then
        echo "Formatting Python script \"$file\"..." 
        black -q $1
    elif (echo $1 | grep -q "CMakeLists.txt") || (echo $1 | grep -q "*.cmake"); then
        echo "Formatting CMake \"$file\"..." 
        cmake-format -c $script_dir/cmake_format_config.py -i $1
    fi
}

print_usage() {
    echo -e "\
${BOLD}Usage:${NONE} ./scripts/format/format.sh [path1] [path2] ... -exclude [keyword1] [keyword2] ...

${BOLD}Supported Languages:${NONE}
    C/C++
    Bash
    Python
    CMake

${BOLD}Examples:${NONE}
    ./scripts/format/format.sh kernel/ipc
    ./scripts/format/format.sh user/system-servers/fsm user/system-servers/procmgr user/init/main.c
    ./scripts/format/format.sh kernel -exclude arch
    ./scripts/format/format.sh kernel -exclude kernel/arch/aarch64
"
}

if [ $# -eq 0 ]; then
    print_usage
    exit
fi

if [ -f /.dockerenv ]; then
    # we are in docker container
    main $@
else
    echo "Starting docker container to do formatting"
    docker run -it --rm \
        -u $(id -u ${USER}):$(id -g ${USER}) \
        -v $(pwd):/chos -w /chos \
        ipads/chcore_formatter:v2.0 \
        ./scripts/format/format.sh $@
fi
