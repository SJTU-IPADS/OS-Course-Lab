# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

function(chcore_copy_binary_to_ramdisk _target)
    add_custom_target(
        cp_${_target}_to_ramdisk
        COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/${_target} ${CHCORE_RAMDISK_DIR}
        DEPENDS ${_target})
    add_dependencies(ramdisk cp_${_target}_to_ramdisk ${_target})
    set_property(GLOBAL PROPERTY ${_target}_INSTALLED TRUE)
endfunction()

function(chcore_copy_target_to_ramdisk _target)
    add_custom_target(
        cp_${_target}_to_ramdisk
        COMMAND cp $<TARGET_FILE:${_target}> ${CHCORE_RAMDISK_DIR}
        DEPENDS ${_target})
    add_dependencies(ramdisk cp_${_target}_to_ramdisk ${_target})
    set_property(GLOBAL PROPERTY ${_target}_INSTALLED TRUE)
endfunction()

# Install all shared library and executable targets defined in
# the current source dir to ramdisk.
#
# This will exclude those that are already installed by
# `chcore_install_target_as_cpio` or `chcore_install_target_to_ramdisk`.
function(chcore_copy_all_targets_to_ramdisk)
    set(_targets)
    chcore_get_all_targets(_targets)
    foreach(_target ${_targets})
        get_property(_installed GLOBAL PROPERTY ${_target}_INSTALLED)
        if(${_installed})
            continue()
        endif()
        get_target_property(_target_type ${_target} TYPE)
        if(${_target_type} STREQUAL SHARED_LIBRARY OR ${_target_type} STREQUAL
                                                      EXECUTABLE)
            chcore_copy_target_to_ramdisk(${_target})
        endif()
    endforeach()
endfunction()

function(chcore_enable_clang_tidy)
    set(_checks
        "-bugprone-easily-swappable-parameters,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling,-bugprone-reserved-identifier"
    )
    set(_options)
    set(_one_val_args EXTRA_CHECKS)
    set(_multi_val_args)
    cmake_parse_arguments(_clang_tidy "${_options}" "${_one_val_args}"
                          "${_multi_val_args}" ${ARGN})
    if(_clang_tidy_EXTRA_CHECKS)
        set(_checks "${_checks},${_clang_tidy_EXTRA_CHECKS}")
    endif()
    set(CMAKE_C_CLANG_TIDY
        clang-tidy --checks=${_checks}
        --extra-arg=-I${CHCORE_MUSL_LIBC_INSTALL_DIR}/include
        --extra-arg=-Qunused-arguments
        --config-file=${PROJECT_SOURCE_DIR}/.clang-tidy
        PARENT_SCOPE)
endfunction()

function(chcore_disable_clang_tidy)
    unset(CMAKE_C_CLANG_TIDY PARENT_SCOPE)
endfunction()

function(chcore_objcopy_binary _user_target _binary_name)
    add_custom_target(
        ${_binary_name} ALL
        COMMAND ${CMAKE_OBJCOPY} -O binary -S $<TARGET_FILE:${_user_target}>
                ${_binary_name}
        DEPENDS ${_user_target})
endfunction()