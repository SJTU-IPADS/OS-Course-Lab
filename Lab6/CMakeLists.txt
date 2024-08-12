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
project(ChCore)

set(_cmake_script_dir ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${_cmake_script_dir}/Modules)
set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR}/user)

include(SubProject)
include(CommonTools)

chcore_dump_chcore_vars()
chcore_collect_pkg_path(${CMAKE_CURRENT_SOURCE_DIR}/user)

chcore_get_nproc(_nproc)

if(CHCORE_VERBOSE_BUILD)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif()

set(_libc_install_dir ${CMAKE_CURRENT_BINARY_DIR}/chcore-libc)
list(APPEND CMAKE_PREFIX_PATH ${_libc_install_dir})

string(REPLACE ";" ":" _colon_prefix_path "${CMAKE_PREFIX_PATH}")
set(_common_args
    -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
    -DCMAKE_PREFIX_PATH=${_colon_prefix_path}
    -DCHCORE_PROJECT_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

# Construct cache args list for subprojects (kernel, user)
macro(chcore_config _config_name _config_type _default _description)
    if(NOT DEFINED ${_config_name})
        message(
            FATAL_ERROR
                "Do not run CMake command directly, use `./chbuild` instead")
    endif()
    list(APPEND _cache_args
         -D${_config_name}:${_config_type}=${${_config_name}})
endmacro()
include(${CMAKE_CURRENT_SOURCE_DIR}/config.cmake)

# --- LibC ---

set(_libc_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/user/chcore-libc)
set(_libc_build_dir ${CMAKE_CURRENT_BINARY_DIR}/user/chcore-libc)

# Main targets for Libc
chcore_add_subproject(
    libc
    SOURCE_DIR ${_libc_source_dir}
    BINARY_DIR ${_libc_build_dir}
    INSTALL_DIR ${_libc_install_dir}
    CMAKE_ARGS
        ${_common_args}
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    CMAKE_CACHE_ARGS ${_cache_args}
    INSTALL_COMMAND echo "Performed in CMake"
    BUILD_ALWAYS TRUE)

# Clean target for LibC
add_custom_target(
    libc-clean-trigger
    WORKING_DIRECTORY ${_libc_build_dir}
    COMMAND [ -d ${_libc_build_dir}/CMakeFiles ] && ${CMAKE_COMMAND} --build . --target  libc-clean || true
    COMMAND /bin/rm -rf ${_libc_install_dir})

# --- Clean Ramdisk ---
# Target to re-create ramdisk, must be run before build user-level stuffs

set(build_ramdisk_dir ${CMAKE_CURRENT_BINARY_DIR}/ramdisk)
file(REMOVE_RECURSE ${build_ramdisk_dir})
file(MAKE_DIRECTORY ${build_ramdisk_dir})


# --- Ramdisk ---

# if(CHCORE_CHPM_INSTALL_TO_RAMDISK AND CHCORE_CHPM_INSTALL_PREFIX)
#     get_filename_component(_chpm_install_prefix ${CHCORE_CHPM_INSTALL_PREFIX}
#                            REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
#     set(_copy_chpm_install_cmd
#         "cp -rL `find ${_chpm_install_prefix} -mindepth 1 -maxdepth 1 -type d -not -name include -and -not -name 'chpm-*'` ."
#     )
#     string(REPLACE " " ";" _copy_chpm_install_cmd ${_copy_chpm_install_cmd})
# else()
#     set(_copy_chpm_install_cmd true)
# endif()

# --- User ---

set(_libs_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/user/chcore-libs)
set(_libs_build_dir ${CMAKE_CURRENT_BINARY_DIR}/user/chcore-libs)
set(_libs_install_dir ${_libs_build_dir})

chcore_add_subproject(
    libs
    SOURCE_DIR ${_libs_source_dir}
    BINARY_DIR ${_libs_build_dir}
    INSTALL_DIR ${_libs_install_dir}
    CMAKE_ARGS
        ${_common_args}
        -DCHCORE_MUSL_LIBC_INSTALL_DIR=${_libc_install_dir} # used by user.cmake toolchain to find `musl-gcc`
        -DCHCORE_RAMDISK_DIR=${build_ramdisk_dir}
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_TOOLCHAIN_FILE=${_cmake_script_dir}/Toolchains/user.cmake
    CMAKE_CACHE_ARGS ${_cache_args}
    INSTALL_COMMAND make install
    DEPENDS libc
    BUILD_ALWAYS TRUE)

set(_apps_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/user/apps)
set(_apps_build_dir ${CMAKE_CURRENT_BINARY_DIR}/user/apps)
set(_apps_install_dir ${_apps_build_dir})

chcore_add_subproject( 
    apps
    SOURCE_DIR ${_apps_source_dir}
    BINARY_DIR ${_apps_build_dir}
    INSTALL_DIR ${_apps_install_dir}
    CMAKE_ARGS
        ${_common_args}
        -DCHCORE_MUSL_LIBC_INSTALL_DIR=${_libc_install_dir} # used by user.cmake toolchain to find `musl-gcc`
        -DCHCORE_RAMDISK_DIR=${build_ramdisk_dir}
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_TOOLCHAIN_FILE=${_cmake_script_dir}/Toolchains/user.cmake
    CMAKE_CACHE_ARGS ${_cache_args}
    INSTALL_COMMAND echo "Nothing to install"
    DEPENDS libs
    BUILD_ALWAYS TRUE)

set(_system_services_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/user/system-services)
set(_system_services_build_dir ${CMAKE_CURRENT_BINARY_DIR}/user/system-services)
set(_system_services_install_dir ${_system_services_build_dir})
# set(_ci_config_dir ${CMAKE_CURRENT_SOURCE_DIR}/scripts/ci)

# We embed a binary into other binaries by creating a source file named incbin_*.S, such files
# need to be re-generated per compilation. So we have to remove them from build dir every time. And that's why it is not included in system-services-clean-trigger
add_custom_target(
    system-services-clean-incbin
    COMMAND
        [ -d ${_system_services_build_dir} ]
        && find ${_system_services_build_dir} -type f -name "incbin_*.S.*" | xargs rm -f
        || true)

# Clean target for system services
add_custom_target(
    system-services-clean-trigger
    WORKING_DIRECTORY ${_system_services_build_dir}
    COMMAND [ -d ${_system_services_build_dir}/CMakeFiles ] && ${CMAKE_COMMAND} --build . --target system-services-clean || true
)

chcore_add_subproject(
    system-services
    SOURCE_DIR ${_system_services_source_dir}
    BINARY_DIR ${_system_services_build_dir}
    INSTALL_DIR ${_system_services_install_dir}
    CMAKE_ARGS
        ${_common_args}
        -DCHCORE_MUSL_LIBC_INSTALL_DIR=${_libc_install_dir} # used by user.cmake toolchain to find `musl-gcc`
        -DCHCORE_RAMDISK_DIR=${build_ramdisk_dir}
        -DCHCORE_CI_CONFIG_DIR=${_ci_config_dir}
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_TOOLCHAIN_FILE=${_cmake_script_dir}/Toolchains/user.cmake
    CMAKE_CACHE_ARGS ${_cache_args}
    INSTALL_COMMAND echo "Nothing to install"
    DEPENDS apps system-services-clean-incbin
    BUILD_ALWAYS TRUE)

# --- Kernel ---

set(_kernel_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/kernel)
set(_kernel_build_dir ${CMAKE_CURRENT_BINARY_DIR}/kernel)
set(_kernel_install_dir ${CMAKE_CURRENT_BINARY_DIR})

# Target to force re-including cpio binaries
add_custom_target(
    kernel-clean-incbin
    COMMAND
        [ -d ${_kernel_build_dir}/CMakeFiles ] && find
        ${_kernel_build_dir}/CMakeFiles -type f -name "incbin_*.S.*" | xargs rm
        -f || true)

# Main targets for kernel
chcore_add_subproject(
    kernel
    SOURCE_DIR ${_kernel_source_dir}
    BINARY_DIR ${_kernel_build_dir}
    INSTALL_DIR ${_kernel_install_dir}
    CMAKE_ARGS
        ${_common_args}
        -DCHCORE_USER_INSTALL_DIR=${_system_services_install_dir} # used by kernel/CMakeLists.txt to incbin cpio files
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_TOOLCHAIN_FILE=${_cmake_script_dir}/Toolchains/kernel.cmake
    CMAKE_CACHE_ARGS ${_cache_args}
    DEPENDS system-services kernel-clean-incbin
    BUILD_ALWAYS TRUE)

# Generate toolchain file
configure_file(${_cmake_script_dir}/Toolchains/toolchain_tpl.cmake 
                toolchain.cmake
                @ONLY)

# Re-generate ramdisk
add_custom_target(
    rebuildproc
    WORKING_DIRECTORY ${_system_services_build_dir}
    COMMAND [ -d ${_system_services_build_dir}/CMakeFiles ] && ${CMAKE_COMMAND} --build . --target buildram || true
)

add_custom_target(
    rambuild
    WORKING_DIRECTORY ${_kernel_build_dir}
    COMMAND [ -d ${_kernel_build_dir}/CMakeFiles ] && ${CMAKE_COMMAND} --build . --target rebuildkernel || true
    DEPENDS rebuildproc
)
# --- Clean All ---

add_custom_target(
    clean-all
)

add_dependencies(clean-all libc-clean-trigger system-services-clean-trigger)

# --- Update Submodules (always run in local env) ---

add_custom_target(update-submodules)

macro(_update_submodule _target _subproject_path _module_rel_path)
    add_custom_target(
        ${_target}
        WORKING_DIRECTORY ${_subproject_path}
        COMMAND echo "Updating ${_subproject_path}/${_module_rel_path}..."
        COMMAND git submodule update --init
                --recursive --depth=1 ${_module_rel_path})
    add_dependencies(update-submodules ${_target})
endmacro()

# _update_submodule(update-musl-libc ${_system_services_source_dir} chcore-libc/musl-libc)

if(CHCORE_DRIVER_FWK_LINUX)
    _update_submodule(update-linux-port ${_system_services_source_dir} system-servers/drivers/linux-port)
endif()
if(CHCORE_DRIVER_FWK_CIRCLE)
    _update_submodule(update-circle ${_system_services_source_dir} system-servers/drivers/raspi/circle)
endif()
if(CHCORE_CROSS_COMPILE MATCHES "riscv64")
    _update_submodule(opensbi ${_kernel_source_dir} arch/riscv64/boot/opensbi)
endif()
