# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

add_compile_definitions(CHCORE)

# Get the target architecture
execute_process(
    COMMAND ${CMAKE_C_COMPILER} -dumpmachine
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE _target_machine)
string(REGEX MATCH "^[^-]+" _target_arch ${_target_machine})

# Set CHCORE_ARCH cache var
# Note: set as cache variable so that it will be passed into C
# as compile definition later
set(CHCORE_ARCH
    ${_target_arch}
    CACHE STRING "" FORCE)

unset(_target_machine)
unset(_target_arch)

# Set optimization level
add_compile_options("$<$<CONFIG:Debug>:-Og;-g>")
add_compile_options("$<$<CONFIG:Release>:-O3>")

# Convert config items to compile definition
get_cmake_property(_cache_var_names CACHE_VARIABLES)
foreach(_var_name ${_cache_var_names})
    string(REGEX MATCH "^CHCORE_" _matched ${_var_name})
    if(NOT _matched)
        continue()
    endif()
    get_property(
        _var_type
        CACHE ${_var_name}
        PROPERTY TYPE)
    if(_var_type STREQUAL BOOL)
        # for BOOL, add definition if ON/TRUE
        if(${_var_name})
            add_compile_definitions(${_var_name})
        endif()
    elseif(_var_type STREQUAL STRING)
        # for STRING, always add definition with string literal value
        add_compile_definitions(${_var_name}="${${_var_name}}")
    endif()
endforeach()

unset(_cache_var_names)
unset(_var_name)
unset(_var_type)
unset(_matched)

# Set CHCORE_ARCH_XXX and CHCORE_PLAT_XXX compile definitions
string(TOUPPER ${CHCORE_ARCH} _arch_uppercase)
string(TOUPPER ${CHCORE_PLAT} _plat_uppercase)
add_compile_definitions(CHCORE_ARCH_${_arch_uppercase}
                        CHCORE_PLAT_${_plat_uppercase})
unset(_arch_uppercase)
unset(_plat_uppercase)

# We use CHCORE_SUBPLAT to avoid redundant code for a new plat
# which is similar to an existing plat.
if(NOT ${CHCORE_SUBPLAT} STREQUAL "")
    string(TOUPPER ${CHCORE_SUBPLAT} _subplat_uppercase)
    add_compile_definitions(CHCORE_SUBPLAT_${_subplat_uppercase})
    unset(_subplat_uppercase)
endif()

# Pass all CHCORE_* variables (cache and non-cache) to
# CMake try_compile projects
get_cmake_property(_var_names VARIABLES)
foreach(_var_name ${_var_names})
    string(REGEX MATCH "^CHCORE_" _matched ${_var_name})
    if(NOT _matched)
        continue()
    endif()
    list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ${_var_name})
endforeach()

unset(_var_names)
unset(_var_name)
unset(_matched)
