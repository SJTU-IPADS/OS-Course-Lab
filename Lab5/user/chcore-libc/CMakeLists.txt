# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

cmake_minimum_required(VERSION 3.14)
project(ChCoreLibc C ASM)

include(CommonTools)
include(LibAppTools)

chcore_get_nproc(_nproc)

set(CMAKE_C_COMPILER "${CHCORE_CROSS_COMPILE}gcc")

set(_cmake_script_dir ${CMAKE_CURRENT_SOURCE_DIR}/libchcore/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${_cmake_script_dir})

include(PortingTools)

execute_process(
    COMMAND ${CMAKE_C_COMPILER} -dumpmachine
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE _target_machine)
string(REGEX MATCH "^[^-]+" _target_arch ${_target_machine})

# Set CHCORE_ARCH cache var
# Note: set as cache variable so that it will be passed into C
# as compile definition later
set(CHCORE_ARCH ${_target_arch})

unset(_target_machine)
unset(_target_arch)

get_cmake_property(cache_vars CACHE_VARIABLES)
set(libc_defs "")
foreach(var ${cache_vars})
    if(var MATCHES "^CHCORE_.*")
        get_property(
            _var_type
            CACHE ${var}
            PROPERTY TYPE)
        if(_var_type STREQUAL BOOL)
            # for BOOL, add definition if ON/TRUE
            if(${var})
                list(APPEND libc_defs -D${var})
            endif()
        elseif(_var_type STREQUAL STRING)
            # for STRING, always add definition with string literal value
            list(APPEND libc_defs -D${var}="${${var}}")
        endif()
    endif()
endforeach()

string(TOUPPER ${CHCORE_ARCH} _arch_uppercase)
string(TOUPPER ${CHCORE_PLAT} _plat_uppercase)
list(APPEND libc_defs -DCHCORE_ARCH_${_arch_uppercase} -DCHCORE_PLAT_${_plat_uppercase})
message(STATUS "libc cdefs: ${libc_defs}")

if(CHCORE_PLAT STREQUAL "bm3823")
    list(APPEND libc_defs -mcpu=v8)
endif()

string(REPLACE ";" " " libc_defs "${libc_defs}")

# Apply overrides and patches from libchcore

set(_libc_target_dir ${CMAKE_CURRENT_SOURCE_DIR}/musl-libc)
set(_libchcore_porting_dir ${CMAKE_CURRENT_SOURCE_DIR}/libchcore/porting)
set(_libchcore_overrides_dir ${_libchcore_porting_dir}/overrides)
set(_libchcore_patches_dir ${_libchcore_porting_dir}/patches)
set(_libchcore_cleanup_script ${CMAKE_CURRENT_SOURCE_DIR}/libchcore/cmake/do_cleanup.sh)

do_overrides(${_libchcore_overrides_dir} ${_libc_target_dir})
do_patches(${_libchcore_patches_dir} ${_libc_target_dir})

# Configure libc and generate syscall.h and alltypes.h
set(_libchcore_arch_includes_dir ${CMAKE_CURRENT_SOURCE_DIR}/libchcore/arch/${CHCORE_ARCH})

set(libc_defs "${libc_defs} -I${_libchcore_arch_includes_dir}")

add_custom_target(libc-configure ALL
    WORKING_DIRECTORY ${_libc_target_dir}
    COMMAND ./configure --prefix=${CMAKE_INSTALL_PREFIX} --syslibdir=${CMAKE_INSTALL_PREFIX}/lib --with-malloc=oldmalloc
    $<$<BOOL:${CHCORE_USER_DEBUG}>:--enable-debug>
    CROSS_COMPILE=${CHCORE_CROSS_COMPILE} COMPILER=${CMAKE_C_COMPILER} $<$<BOOL:${libc_defs}>:CFLAGS=${libc_defs}>
    UAPI_INCLUDE=${CHCORE_PROJECT_DIR}/kernel/user-include
)

# Compile libc
add_custom_target(libc-build ALL
    WORKING_DIRECTORY ${_libc_target_dir}
    COMMAND bear make -j${_nproc}
    DEPENDS libc-configure)

# Install libc as usual
add_custom_target(libc-install ALL
    WORKING_DIRECTORY ${_libc_target_dir}
    COMMAND make -j1 install
    DEPENDS libc-build)

# clean target
add_custom_target(libc-clean
    COMMAND make -C ${_libc_target_dir} clean
    COMMAND bash ${_libchcore_cleanup_script} ${_libc_target_dir})
