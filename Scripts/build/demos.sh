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

# TODO(qyc): This file should be removed when ChPM can install
# demo applications from source.

. ${TOP_DIR}/scripts/util/util.sh

build_demo_libs() {
    set -e

    # Build libvideo
    echo "Build libvideo... wait for a while..."
    enter_dir "${DEMOS_DIR}/libvideo"
    make CC=$USER_DIR/chcore-libc/build/bin/musl-gcc -j4 -s
    leave_dir

    # Build minigui libs && helloworld.bin
    echo "Build MiniGUI... wait for a while..."
    enter_dir "${DEMOS_DIR}/minigui"
    ./buildlib-chcore &> /dev/null
    make -j4 -s &> /dev/null
    leave_dir
}

build_demos() {
    set -e

    # Redis
    echo "Build Redis... wait for one minute ..."
    
    enter_dir "${DEMOS_DIR}/redis-6.0.8"
    make MALLOC=libc CC=$USER_DIR/chcore-libc/build/bin/musl-gcc -j4 -s &> /dev/null
    leave_dir

    echo "Done."

    enter_dir "${DEMOS_DIR}/redis-6.0.8/src"

    mv -f redis-cli redis-cli.bin
    mv -f redis-server redis-server.bin
    mv -f redis-benchmark redis-benchmark.bin

    # lvgl
    echo "Build lvgl... wait for a while..."

    enter_dir "${DEMOS_DIR}/lvgl"

    cmake -S . -B build &> /dev/null
    cmake --build build -- -j8 &> /dev/null

    leave_dir

    echo "Done."

    # Darknet
    echo "Build darknet... wait for a while..."
    
    enter_dir "${DEMOS_DIR}/darknet"
    make CC=$USER_DIR/chcore-libc/build/bin/musl-gcc &> /dev/null
    leave_dir

    echo "Done."

    # Sqlite3
    echo "Build Sqlite3... wait for a while..."
    
    enter_dir "${DEMOS_DIR}/sqlite"
    make CC=$USER_DIR/chcore-libc/build/bin/musl-gcc -s &> /dev/null
    leave_dir

    echo "Done."

    # Lighttpd
    echo "Build Lighttpd... wait for a while..."
    
    enter_dir "${DEMOS_DIR}/lighttpd-1.4.57"

    cmake -S . -B build &> /dev/null
    cmake --build build -- -j8 &> /dev/null
    mv -f build/build/lighttpd build/build/lighttpd.bin

    leave_dir

    echo "Done."

    # Machine-ARM
    echo "Build Machine Arm... wait for a while..."
    
    enter_dir "${DEMOS_DIR}/machine_arm"
    make CC=$USER_DIR/chcore-libc/build/bin/musl-gcc -s &> /dev/null
    leave_dir

    echo "Done."

    # MiniGUI
    if [[ ${CHCORE_ARCH} != *"x86_64"* ]]; then
        echo "Build MiniGUI Apps... wait for a while..."
        
        enter_dir "${DEMOS_DIR}/minigui/examples"

        cmake -S . -B build &> /dev/null
        cmake --build build -- -j8 &> /dev/null

        leave_dir

        echo "Done."
    fi

    # Iozone
    echo "Build Iozone... wait for a while..."

    enter_dir "${DEMOS_DIR}/iozone"
    make CC=$USER_DIR/chcore-libc/build/bin/musl-gcc -s linux &> /dev/null
    leave_dir

    echo "Done."
}

copy_demo_libs_to_ramdisk() {
    enter_dir "${USER_DIR}/build"

    echo "copy libminigui to ramdisk."
    cp $DEMOS_DIR/minigui/src/.libs/libminigui_ths-4.0.so.0.0.8 ramdisk/libminigui_ths-4.0.so.0

    echo "copy libvideo to ramdisk."
    cp $DEMOS_DIR/libvideo/*.so ramdisk/

    leave_dir
}

copy_demos_to_ramdisk() {
    enter_dir "${USER_DIR}/build"

    echo "copy existing demo apps to ramdisk"
    cp $DEMOS_DIR/redis-6.0.8/src/*.bin ramdisk/ 2>/dev/null || :
    cp $DEMOS_DIR/sqlite/*.bin ramdisk/ 2>/dev/null || :
    cp $DEMOS_DIR/lighttpd-1.4.57/build/build/*.bin ramdisk/ 2>/dev/null || :
    cp -R $DEMOS_DIR/www_test ramdisk/www 2>/dev/null || :
    cp $DEMOS_DIR/machine_arm/*.bin ramdisk/ 2>/dev/null || :
    cp $DEMOS_DIR/darknet/*.bin ramdisk/ 2>/dev/null || :
    cp $DEMOS_DIR/darknet/tiny.weights ramdisk/ 2>/dev/null || :
    cp -R $DEMOS_DIR/darknet/cfg ramdisk/cfg 2>/dev/null || :
    cp -R $DEMOS_DIR/darknet/data ramdisk/data 2>/dev/null || :
    cp $DEMOS_DIR/minigui/examples/build/mg_helloworld.bin ramdisk/ 2>/dev/null || :
    cp $DEMOS_DIR/minigui/examples/build/minesweeper/mg_minesweeper.bin ramdisk/ 2>/dev/null || :
    cp $DEMOS_DIR/minigui/examples/build/notebook/mg_notebook.bin ramdisk/ 2>/dev/null || :
    cp -R $DEMOS_DIR/minigui/examples/minesweeper/res ramdisk/ 2>/dev/null || :
    cp -R $DEMOS_DIR/minigui/examples/notebook/res ramdisk/ 2>/dev/null || :
    cp $DEMOS_DIR/iozone/iozone ramdisk/iozone.bin 2>/dev/null || :
    cp -R $DEMOS_DIR/lvgl/build/test_lvgl.bin ramdisk/ 2>/dev/null || :
    cp -R $DEMOS_DIR/lvgl/build/test_widgets_lvgl.bin ramdisk/ 2>/dev/null || :

    leave_dir
}

clean_demos() {
    echo "Clean demos start ..."

    if [[ -d ${DEMOS_DIR}/libvideo && -f "${DEMOS_DIR}/libvideo/Makefile" ]]; then
        # Libvideo
        enter_dir "${DEMOS_DIR}/libvideo"
        make clean &> /dev/null
        leave_dir
    fi

    # Redis
    if [[ -d ${DEMOS_DIR}/redis-6.0.8 && -f "${DEMOS_DIR}/redis-6.0.8/Makefile" ]]; then
        enter_dir "${DEMOS_DIR}/redis-6.0.8"
        make distclean &> /dev/null
        make clean &> /dev/null
        leave_dir
    fi

    # Darknet
    if [[ -d ${DEMOS_DIR}/darknet && -f "${DEMOS_DIR}/darknet/Makefile" ]]; then
        enter_dir "${DEMOS_DIR}/darknet"
        make clean &> /dev/null
        leave_dir
    fi

    # Sqlite
    if [[ -d ${DEMOS_DIR}/sqlite && -f "${DEMOS_DIR}/sqlite/Makefile" ]]; then
        enter_dir "${DEMOS_DIR}/sqlite"
        make clean &> /dev/null
        leave_dir
    fi

    # Lighttpd
    if [[ -d ${DEMOS_DIR}/lighttpd-1.4.57 ]]; then
        enter_dir "${DEMOS_DIR}/lighttpd-1.4.57"
        rm -rf build
        leave_dir
    fi

    # Machine ARM
    if [[ -d ${DEMOS_DIR}/machine_arm && -f "${DEMOS_DIR}/machine_arm/Makefile" ]]; then
        enter_dir "${DEMOS_DIR}/machine_arm"
        make clean &> /dev/null
        leave_dir
    fi

    # MiniGUI
    if [[ -d ${DEMOS_DIR}/minigui/examples ]]; then
        enter_dir "${DEMOS_DIR}/minigui/examples"
        rm -rf build
        leave_dir
    fi

    # iozone
    if [[ -d ${DEMOS_DIR}/iozone && -f "${DEMOS_DIR}/iozone/makefile" ]]; then
        enter_dir "${DEMOS_DIR}/iozone"
        make clean &> /dev/null
        leave_dir
    fi

    # lvgl
    if [[ -d ${DEMOS_DIR}/lvgl ]]; then
        enter_dir "${DEMOS_DIR}/lvgl"
        rm -rf build
        leave_dir
    fi

    echo "Clean demos finished"
}