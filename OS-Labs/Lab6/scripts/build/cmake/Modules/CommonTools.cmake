# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

function(chcore_dump_cmake_vars)
    message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
    message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
    message(STATUS "CMAKE_CROSSCOMPILING: ${CMAKE_CROSSCOMPILING}")
    message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
    message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
    message(
        STATUS "CMAKE_HOST_SYSTEM_PROCESSOR: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
    message(STATUS "CMAKE_HOST_SYSTEM_NAME: ${CMAKE_HOST_SYSTEM_NAME}")
    message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
    message(STATUS "CMAKE_ASM_COMPILER: ${CMAKE_ASM_COMPILER}")
    message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
    message(STATUS "CMAKE_C_OUTPUT_EXTENSION: ${CMAKE_C_OUTPUT_EXTENSION}")
    message(STATUS "CMAKE_LINKER: ${CMAKE_LINKER}")
    message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
    message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
    message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
    message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")

    chcore_dump_chcore_vars()
endfunction()

function(chcore_dump_chcore_vars)
    get_cmake_property(_variable_names VARIABLES)
    list(SORT _variable_names)
    foreach(_variable_name ${_variable_names})
        string(REGEX MATCH "^CHCORE_" _matched ${_variable_name})
        if(NOT _matched)
            continue()
        endif()
        message(STATUS "${_variable_name}: ${${_variable_name}}")
    endforeach()
endfunction()

macro(chcore_config_include _config_rel_path)
    include(${CMAKE_CURRENT_LIST_DIR}/${_config_rel_path})
endmacro()

function(chcore_target_remove_compile_options _target)
    get_target_property(_target_options ${_target} COMPILE_OPTIONS)
    if(_target_options)
        foreach(_option ${ARGN})
            list(REMOVE_ITEM _target_options ${_option})
        endforeach()
        set_target_properties(${_target} PROPERTIES COMPILE_OPTIONS
                                                    "${_target_options}")
    endif()
endfunction()

function(chcore_target_remove_link_options _target)
    get_target_property(_target_options ${_target} LINK_OPTIONS)
    if(_target_options)
        message(STATUS "remove link options before: ${_target_options}")
        foreach(_option ${ARGN})
            list(REMOVE_ITEM _target_options ${_option})
        endforeach()
        set_target_properties(${_target} PROPERTIES LINK_OPTIONS
                                                    "${_target_options}")
        message(STATUS "remove link options after: ${_target_options}")
    endif()
endfunction()

if(NOT COMMAND ProcessorCount)
    include(ProcessorCount)
endif()

macro(chcore_get_nproc _nproc)
    ProcessorCount(${_nproc})
    if(${_nproc} EQUAL 0)
        set(${_nproc} 16)
    endif()
endmacro()

# Get all "build system targets" defined in the current source dir,
# recursively.
function(chcore_get_all_targets _out_var)
    set(_targets)
    _get_all_targets_recursive(_targets ${CMAKE_CURRENT_SOURCE_DIR})
    set(${_out_var}
        ${_targets}
        PARENT_SCOPE)
endfunction()

macro(_get_all_targets_recursive _targets _dir)
    get_property(
        _subdirectories
        DIRECTORY ${_dir}
        PROPERTY SUBDIRECTORIES)
    foreach(_subdir ${_subdirectories})
        _get_all_targets_recursive(${_targets} ${_subdir})
    endforeach()

    get_property(
        _current_targets
        DIRECTORY ${_dir}
        PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND ${_targets} ${_current_targets})
endmacro()

macro(chcore_collect_pkg_path _dir)
    file(GLOB children ${_dir}/*)
    foreach(child ${children})
        if(IS_DIRECTORY ${child})
            list(APPEND CMAKE_PREFIX_PATH ${child})
        endif()
    endforeach()
endmacro()

function(chcore_target_dynamic_linked_exec _target)
    chcore_target_remove_compile_options(${_target} -fPIC)
    chcore_target_remove_link_options(${_target} -pic -static)
    target_compile_options(${_target} PRIVATE -fPIE)
    target_link_options(${_target} PRIVATE -pie)
endfunction()

function(chcore_target_force_static_linked _target)
    chcore_target_remove_compile_options(${_target} -fPIC -fPIE)
    chcore_target_remove_link_options(${_target} -pic -pie)
    target_link_options(${_target} PRIVATE -static)
    set_target_properties(${_target} PROPERTIES forced_static TRUE)
endfunction()

function(chcore_all_dynamic_linked_exec)
    chcore_get_all_targets(_targets)
    foreach(_target ${_targets})
        get_target_property(_target_type ${_target} TYPE)
        get_target_property(_target_forced_static ${_target} forced_static)
        if(_target_type STREQUAL EXECUTABLE AND NOT _target_forced_static)
            chcore_target_dynamic_linked_exec(${_target})
        endif()
    endforeach()
endfunction()

function(chcore_all_force_static_linked)
    chcore_get_all_targets(_targets)
    foreach(_target ${_targets})
        get_target_property(_target_type ${_target} TYPE)
        if(_target_type STREQUAL EXECUTABLE)
            chcore_target_force_static_linked(${_target})
        endif()
    endforeach()
endfunction()