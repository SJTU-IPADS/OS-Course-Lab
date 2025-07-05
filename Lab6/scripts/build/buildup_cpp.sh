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

# The .cpp directory structure:
#       .cpp
#       ├── aarch64
#       │   ├── include
#       │   └── lib
#       ├─── x86_64
#       │   ├── include
#       │   └── lib
#       ├─── riscv64
#       │   ├── include
#       │   └── lib
#       └─ download
#           ├── musl-cross-make
#           └── prebuild
# 
# When using C++ toolchains, {arch}/include & {arch}/include/{musl_target} directory 
# is included into prefix path by CMKAE function `include_directories`
#
# {arch}/lib/libgcc_s.so.1 & {arch}/lib/libstdc++.so.6 file is copy into ramdisk directory
# to support staros C++ programs in runtime

set -e

project_dir=$(cd $(dirname $(dirname $(cd $(dirname "$0"); pwd))); pwd)
cpp_dir="$project_dir/.cpp"
download_dir="$project_dir/.cpp/download/"

libfile_url="git@ipads.se.sjtu.edu.cn:staros/cpp-libs.git"

musl_cross_dir="$download_dir/musl-cross-make"
musl_cross_url="https://github.com/richfelker/musl-cross-make.git"

GCC_VER=9.2.0
MUSL_VER=1.2.3

copy_to_ramdisk() {
        arch=$1

        if [ ! -d $project_dir/ramdisk ]; then
                mkdir $project_dir/ramdisk
        fi
        
        cp $cpp_dir/$arch/lib/libgcc_s.so.1 $project_dir/ramdisk
        cp $cpp_dir/$arch/lib/libstdc++.so.6 $project_dir/ramdisk
}

prebuild_load() {
        retval=1

        if [ ! -d "$cpp_dir" ]; then
                mkdir $cpp_dir
                mkdir $download_dir
        fi

        # Get the package from the target URL
        if [ ! -d "$download_dir/prebuild" ]; then
                git clone --depth 1 $libfile_url $download_dir/prebuild
        fi

        # if prebuild load success, exit
        if [ -d "$download_dir/prebuild" ]; then
                cp -r $download_dir/prebuild/* $cpp_dir

                copy_to_ramdisk $1

                retval=0
        fi

        return $retval
}

musl_cross_build() {
        pushd $musl_cross_dir

        echo "TARGET = $1" > config.mak
        echo "GCC_VER=$GCC_VER" >> config.mak
        echo "MUSL_VER=$MUSL_VER" >> config.mak

        make -j$(nproc)

        rm config.mak

        popd
}

#usage: musl_cross_install $musl_target $arch
musl_cross_install() {
        musl_target=$1
        target_arch=$2

        musl_target_dir=$musl_cross_dir/build/local/$musl_target
        musl_libsupcpp_dir=$musl_target_dir/src_gcc/libstdc++-v3/libsupc++
        cpp_target_dir=$cpp_dir/$target_arch

        if [ -d "$cpp_target_dir" ]; then
                rm -rf $cpp_target_dir
        fi

        mkdir $cpp_target_dir
        mkdir $cpp_target_dir/lib

        cp -Lr $musl_target_dir/obj_gcc/$musl_target/libstdc++-v3/include $cpp_target_dir/include
        cp $musl_libsupcpp_dir/exception $cpp_target_dir/include/
        cp $musl_libsupcpp_dir/typeinfo $cpp_target_dir/include/
        cp $musl_libsupcpp_dir/new $cpp_target_dir/include/
        cp $musl_libsupcpp_dir/initializer_list $cpp_target_dir/include/

        cp $musl_target_dir/obj_gcc/$musl_target/libstdc++-v3/src/.libs/libstdc++.so* $cpp_dir/$2/lib
        cp $musl_target_dir/obj_gcc/gcc/libgcc_s.so* $cpp_dir/$2/lib

        copy_to_ramdisk $2
}


_main() {
        case $1 in

        x86_64)
                target_arch=x86_64
                musl_target=x86_64-linux-musl
                ;;
        aarch64 | raspi3 | raspi4)
                target_arch=aarch64
                musl_target=aarch64-linux-musleabi
                ;;
        riscv64)
                target_arch=riscv64
                musl_target=riscv64-linux-musl
                ;;
        *)
                echo "Usage: $0 target_arch"
                exit
                ;;
        esac

        prebuild_load $target_arch
        if [ $? -eq 0 ]; then
                echo "Done! You can try to build the cpp project now!"
                exit 0
        fi


        read -n 1 -p "git clone from $libfile_url failed, if you want to build from source?[y/n] :" response

        if [ ! "$response" == "y" ]; then
                echo "exit!"
                exit 0
        fi

        #if prebuild load failed, build from source
        if [ ! -d $musl_cross_dir ]; then
                git clone $musl_cross_url $musl_cross_dir
        fi

        if [ ! -d $musl_cross_dir ]; then
                echo "git clone failed"
                exit 1
        fi

        musl_cross_build $musl_target
        musl_cross_install $musl_target $target_arch

        echo "Done! You can try to build the cpp project now!"
        exit 0
}

_main $@
