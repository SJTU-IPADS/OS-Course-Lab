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
project(ChCoreKernel C ASM)

include(CommonTools)
include(KernelTools)

chcore_dump_cmake_vars()

# Create kernel.img target
set(kernel_target "kernel.img")
add_executable(${kernel_target})
install(TARGETS ${kernel_target} DESTINATION ${CMAKE_INSTALL_PREFIX})

# Set warning level
list(APPEND _compile_options -Wall)
list(APPEND _compile_options -Werror)

# Generic compile settings
list(APPEND _compile_definitions)
list(APPEND _compile_options -nostdinc -ffreestanding)
list(APPEND _asm_compile_definitions __ASM__)
list(APPEND _asm_compile_options)
list(APPEND _c_compile_definitions)
list(APPEND _c_compile_options)

# Arch-specific compile settings
if(CHCORE_ARCH STREQUAL "aarch64")
    if(NOT FBINFER) # `armv8-a+nofp` is not supported by fbinfer
        list(APPEND _compile_options -march=armv8-a+nofp)
    endif()
elseif(CHCORE_ARCH STREQUAL "riscv64")
    list(
        APPEND
        _compile_options
        -mno-fdiv
        -mno-relax
        -march=rv64imafd
        -mabi=lp64
        -mcmodel=medany)
elseif(CHCORE_ARCH STREQUAL "x86_64")
    list(APPEND _compile_options -mno-sse -mfsgsbase -fno-stack-protector)
elseif(CHCORE_ARCH STREQUAL "sparc")
    if(CHCORE_PLAT STREQUAL "leon3")
        list(APPEND _compile_options -mcpu=leon3)
        list(APPEND _compile_definitions PLAT_HAVE_CASA)
    elseif(CHCORE_PLAT STREQUAL "bm3823")
        list(APPEND _compile_definitions SAVE_FPU_ON_DISABLE)
        list(APPEND _compile_options -mcpu=v8)
    endif()
    list(APPEND _compile_definitions BIG_ENDIAN)
endif()

# Arch-specific compile settings for code model
if (CHCORE_ARCH STREQUAL "sparc")
    list(APPEND _compile_options -fno-pic -fno-pie -mno-fpu)
elseif (CHCORE_ARCH STREQUAL "riscv64")
    list(APPEND _compile_options -fPIC)
else()
    if(CHCORE_PLAT STREQUAL "ft2000")
        list(APPEND _compile_options -fPIC)
    else()
        list(APPEND _compile_options -fno-pic -fno-pie -mcmodel=large)
    endif()
endif()


# This is only for `fbinfer` stage in CI script
if(FBINFER)
    list(APPEND _compile_definitions FBINFER)
endif()

# Set compile settings to target
target_compile_definitions(${kernel_target} PRIVATE ${_compile_definitions})
target_compile_options(${kernel_target} PRIVATE ${_compile_options})
target_compile_definitions(
    ${kernel_target}
    PRIVATE $<$<COMPILE_LANGUAGE:ASM>:${_asm_compile_definitions}>)
target_compile_options(
    ${kernel_target} PRIVATE $<$<COMPILE_LANGUAGE:ASM>:${_asm_compile_options}>)
target_compile_definitions(
    ${kernel_target} PRIVATE $<$<COMPILE_LANGUAGE:C>:${_c_compile_definitions}>)
target_compile_options(${kernel_target}
                       PRIVATE $<$<COMPILE_LANGUAGE:C>:${_c_compile_options}>)

# Linker options
target_link_options(${kernel_target} PRIVATE -no-pie -nostdlib -nostartfiles
                    -Wl,--build-id=none)

# Build-config-specific options
# FIXME(qyc): defining with hardcoded values is ugly
target_compile_definitions(${kernel_target}
                           PRIVATE $<$<CONFIG:Debug>:LOG_LEVEL=2>)
target_compile_definitions(${kernel_target}
                           PRIVATE $<$<CONFIG:Release>:LOG_LEVEL=1>)

# Include directories
target_include_directories(${kernel_target} PRIVATE include)
target_include_directories(${kernel_target} PRIVATE user-include)
target_include_directories(${kernel_target} PRIVATE include/arch/${CHCORE_ARCH})
target_include_directories(
    ${kernel_target} PRIVATE include/arch/${CHCORE_ARCH}/plat/${CHCORE_PLAT})

# Add source code directories
add_subdirectory(arch/${CHCORE_ARCH})
add_subdirectory(arch/${CHCORE_ARCH}/plat/${CHCORE_PLAT})
add_subdirectory(ipc)
add_subdirectory(irq)
add_subdirectory(lib)
add_subdirectory(mm)
add_subdirectory(object)
add_subdirectory(sched)
add_subdirectory(syscall)

# Kernel testing
if(CHCORE_KERNEL_TEST)
    add_subdirectory(tests/runtime)
endif()

macro(_kernel_incbin _binary_name _binary_path)
    set(binary_name ${_binary_name})
    set(binary_path ${_binary_path})
    configure_file(incbin.tpl.S incbin_${_binary_name}.S)
    unset(binary_name)
    unset(binary_path)
    
target_sources(${kernel_target} PRIVATE incbin_procmgr_bin.S)
endmacro()

# Include essential user-level binaries
_kernel_incbin(procmgr_bin ${CHCORE_USER_INSTALL_DIR}/system-servers/procmgr/procmgr)

# Rebuild kernel when ramdisk changes
add_custom_target(touch-inc-procmgr
                COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/incbin_procmgr_bin.S)

add_custom_target(rebuildkernel
                COMMAND make install
                DEPENDS ${kernel_target})

add_dependencies(${kernel_target} touch-inc-procmgr)
