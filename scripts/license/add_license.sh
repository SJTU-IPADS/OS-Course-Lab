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

export LICENSE_C_STYLE=$(dirname $0)/license_c_style.txt
export LICENSE_SH_STYLE=$(dirname $0)/license_sh_style.txt
export COPYRIGHT_STRING="Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)"
export TMP_FILE=/tmp/$(uuidgen)

_add_license() {
    # Skip empty files due to libchcore overriding rules
    if ! [[ -s $1 ]] ; then
        exit
    fi

    if grep -q "$COPYRIGHT_STRING" $1 ; then
        exit
    fi

    case $(basename $1) in
    *.c | *.cpp | *.h | *.h.in | *.s | *.S | *.ld)
        license=$LICENSE_C_STYLE
        ;;
    *.sh | *.py | *.cmake | *.cmake.in | CMakeLists.txt | chbuild | chpm)
        license=$LICENSE_SH_STYLE
        ;;
    *)
        ;;
    esac

    if [[ $license ]] ; then
        hashbang=$(head -n1 $1 | grep "^#!")
        
        if [[ $hashbang ]] ; then
            sed -i '1d' $1 # Remove hashbang
            sed -i '/./,$!d' $1 # Remove prefix empty lines
            echo $hashbang > $TMP_FILE # Add hashbang to the result file 
        fi

        cat $license $1 >> $TMP_FILE
        chmod $(stat -c "%a" $1) $TMP_FILE
        mv $TMP_FILE $1
    fi
}
export -f _add_license

find . -type f \
    -not -path "*.git*" \
    -not -path "*.repo*" \
    -not -path "*musl-libc*" \
    -not -path "*opensbi*" \
    -not -path "*circle*" \
    -not -path "*linux-port*" \
    -not -path "*littlefs*" \
    | xargs -r -I{} bash -c "_add_license {}"
