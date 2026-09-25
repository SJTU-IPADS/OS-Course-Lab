# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

# CMake toolchain for building ChCore user-level libs and apps.

if(NOT DEFINED CHCORE_PROJECT_DIR)
    message(FATAL_ERROR "CHCORE_PROJECT_DIR is not defined")
else()
    message(STATUS "CHCORE_PROJECT_DIR: ${CHCORE_PROJECT_DIR}")
endif()

if(NOT DEFINED CHCORE_MUSL_LIBC_INSTALL_DIR)
    message(FATAL_ERROR "CHCORE_MUSL_LIBC_INSTALL_DIR is not defined")
else()
    message(
        STATUS "CHCORE_MUSL_LIBC_INSTALL_DIR: ${CHCORE_MUSL_LIBC_INSTALL_DIR}")
endif()

# Set toolchain executables
set(CMAKE_ASM_COMPILER "${CHCORE_MUSL_LIBC_INSTALL_DIR}/bin/musl-gcc")
set(CMAKE_C_COMPILER "${CHCORE_MUSL_LIBC_INSTALL_DIR}/bin/musl-gcc")
set(CMAKE_CXX_COMPILER "${CHCORE_MUSL_LIBC_INSTALL_DIR}/bin/musl-gcc")
set(CMAKE_AR "${CHCORE_MUSL_LIBC_INSTALL_DIR}/bin/musl-ar")
set(CMAKE_NM "${CHCORE_CROSS_COMPILE}nm")
set(CMAKE_OBJCOPY "${CHCORE_CROSS_COMPILE}objcopy")
set(CMAKE_OBJDUMP "${CHCORE_CROSS_COMPILE}objdump")
set(CMAKE_RANLIB "${CHCORE_CROSS_COMPILE}ranlib")
set(CMAKE_STRIP "${CHCORE_MUSL_LIBC_INSTALL_DIR}/bin/musl-strip")

# Set build type
if(CHCORE_USER_DEBUG)
    set(CMAKE_BUILD_TYPE "Debug")
else()
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Build position independent code, a.k.a -fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(${CMAKE_CURRENT_LIST_DIR}/_common.cmake)

# Set the target system (automatically set CMAKE_CROSSCOMPILING to true)
set(CMAKE_SYSTEM_NAME "ChCore")
set(CMAKE_SYSTEM_PROCESSOR ${CHCORE_ARCH})

# Set prefix path
if(CHCORE_CHPM_INSTALL_PREFIX)
    # Get absolute path
    get_filename_component(_chpm_install_prefix ${CHCORE_CHPM_INSTALL_PREFIX}
                           REALPATH BASE_DIR ${CHCORE_PROJECT_DIR})

    string(REPLACE ":" ";" CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}")
    # For find_package, find_library, etc.
    list(APPEND CMAKE_PREFIX_PATH ${_chpm_install_prefix})

    # C++ headers (FIXME: now we hardcode the version number)
    if(CHCORE_ARCH STREQUAL "x86_64")
        include_directories(
            $<$<COMPILE_LANGUAGE:CXX>:${_chpm_install_prefix}/include/c++/9.2.0/x86_64-linux-musl>
        )
    elseif(CHCORE_ARCH STREQUAL "aarch64")
        include_directories(
            $<$<COMPILE_LANGUAGE:CXX>:${_chpm_install_prefix}/include/c++/9.2.0/aarch64-linux-musleabi>
        )
    elseif(CHCORE_ARCH STREQUAL "riscv64")
        include_directories(
            $<$<COMPILE_LANGUAGE:CXX>:${_chpm_install_prefix}/include/c++/9.2.0/riscv64-linux-musl>
    )
    else()
        message(
            WARNING
                "Please set arch-specific C++ header location for ${CHCORE_ARCH}"
        )
    endif()
    include_directories(
        $<$<COMPILE_LANGUAGE:CXX>:${_chpm_install_prefix}/include/c++/9.2.0>)

    # Link C++ standard library for C++ apps
    if(EXISTS ${_chpm_install_prefix}/lib/libstdc++.so)
        set(CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} -L${_chpm_install_prefix}/lib/ ${_chpm_install_prefix}/lib/libstdc++.so ${_chpm_install_prefix}/lib/libgcc_s.so"
        )
    endif()
endif()

# FIXME: check if this applies to all cmake targets
if(CHCORE_PLAT STREQUAL "bm3823")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=v8")
endif()

if(CHCORE_CPP_INSTALL_PREFIX)
    # Get absolute path
    get_filename_component(_cpp_install_prefix ${CHCORE_CPP_INSTALL_PREFIX}
                            REALPATH BASE_DIR ${CHCORE_PROJECT_DIR})

    string(REPLACE ":" ";" CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}")
    # For find_package, find_library, etc.
    list(APPEND CMAKE_PREFIX_PATH ${_cpp_install_prefix})

    # Check CXX toolchain directory
    if((NOT EXISTS ${_cpp_install_prefix}) AND CHCORE_APP_SAMPLE_CXX)
        message(SEND_ERROR "To use C++ toolchain, please setup ${CHCORE_CPP_INSTALL_PREFIX} directory first!\nYou can run ./scripts/build/buildup_cpp.sh {arch} to buildup c++ toolchain!")
    endif()

    # C++ headers (FIXME: now we hardcode the version number)
    if(CHCORE_ARCH STREQUAL "x86_64")
        include_directories(
            $<$<COMPILE_LANGUAGE:CXX>:${_cpp_install_prefix}/x86_64/include/x86_64-linux-musl>
        )
    elseif(CHCORE_ARCH STREQUAL "aarch64")
        include_directories(
            $<$<COMPILE_LANGUAGE:CXX>:${_cpp_install_prefix}/aarch64/include/aarch64-linux-musleabi>
        )
    elseif(CHCORE_ARCH STREQUAL "riscv64")
        include_directories(
            $<$<COMPILE_LANGUAGE:CXX>:${_cpp_install_prefix}/riscv64/include/riscv64-linux-musl>
    )
    else()
        message(
            WARNING
                "Please set arch-specific C++ header location for ${CHCORE_ARCH}"
        )
    endif()
    include_directories(
        $<$<COMPILE_LANGUAGE:CXX>:${_cpp_install_prefix}/${CHCORE_ARCH}/include>)

    # Link C++ standard library for C++ apps
    if(EXISTS ${_cpp_install_prefix}/${CHCORE_ARCH}/lib/libstdc++.so)
        set(CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} -L${_cpp_install_prefix}/${CHCORE_ARCH}/lib/ ${_cpp_install_prefix}/${CHCORE_ARCH}/lib/libstdc++.so ${_cpp_install_prefix}/${CHCORE_ARCH}/lib/libgcc_s.so"
        )
    endif()
endif()
