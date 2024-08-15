# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

function(chcore_copy_target_to_ramdisk _target)
    add_custom_target(
        cp_${_target}_to_ramdisk ALL
        COMMAND cp $<TARGET_FILE:${_target}> ${CHCORE_RAMDISK_DIR}
        DEPENDS ${_target})
    set_property(GLOBAL PROPERTY ${_target}_INSTALLED TRUE)
endfunction()

function(chcore_copy_target_to_ramdisk_api_tests _target)
    set(API_DIR ${CHCORE_RAMDISK_DIR}/api_tests)
    add_custom_target(
        cp_${_target}_to_ramdisk_api_test ALL
        COMMAND cp $<TARGET_FILE:${_target}> ${API_DIR}
        DEPENDS ${_target})
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

function(chcore_copy_all_targets_to_ramdisk_api_tests)
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
            chcore_copy_target_to_ramdisk_api_tests(${_target})
        endif()
    endforeach()
endfunction()

function(chcore_copy_files_to_ramdisk)
    file(COPY ${ARGN} DESTINATION ${CHCORE_RAMDISK_DIR})
endfunction()

include(GNUInstallDirs)

function(chcore_target_include_and_export_directory _target _type _dir)
    target_include_directories(${_target}
        ${_type}
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${_dir}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
    file(GLOB _include_headers "${_dir}/*.h")
    install(FILES ${_include_headers}
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    file(GLOB children ${_dir}/*)
    foreach(child ${children})
        if(IS_DIRECTORY ${child})
            install(DIRECTORY ${child}
                    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
        endif()
    endforeach()
endfunction()

function(chcore_import_library _target _type _filename)
    add_library(${_target} INTERFACE)
    target_link_libraries(${_target} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${_filename})
endfunction()

function(chcore_export_libs_as_package _pkg_name)
    include(CMakePackageConfigHelpers)
    set(_targets)
    set(_exported_targets)
    set(_exported FALSE)
    chcore_get_all_targets(_targets)
    get_property(imported_targets GLOBAL PROPERTY CUSTOM_IMPORTED_TARGETS)
    list(APPEND _targets ${imported_targets})
    foreach(_target ${_targets})
        get_target_property(_target_type ${_target} TYPE)
        if(${_target_type} STREQUAL STATIC_LIBRARY 
                OR ${_target_type} STREQUAL OBJECT_LIBRARY
                OR ${_target_type} STREQUAL SHARED_LIBRARY
                OR ${_target_type} STREQUAL INTERFACE_LIBRARY)
            set(_exported TRUE)
            install(TARGETS ${_target} 
                    EXPORT _exported_targets
                    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
        endif()
    endforeach()
    if(${_exported})
        install(EXPORT _exported_targets
            FILE ${_pkg_name}_targets.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${_pkg_name})
        configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
            "${CMAKE_CURRENT_BINARY_DIR}/${_pkg_name}-config.cmake.tmp"
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${_pkg_name})
        install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${_pkg_name}-config.cmake.tmp"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${_pkg_name}
            RENAME ${_pkg_name}-config.cmake)
    endif()
endfunction()

function(chcore_export_executables_as_package _pkg_name)
    include(CMakePackageConfigHelpers)
    set(_targets)
    set(_exported_targets)
    set(_exported FALSE)
    chcore_get_all_targets(_targets)
    foreach(_target ${_targets})
        get_target_property(_target_type ${_target} TYPE)
        if(${_target_type} STREQUAL EXECUTABLE)
            set(_exported TRUE)
            install(TARGETS ${_target} 
                    EXPORT _exported_targets
                    DESTINATION ${CMAKE_INSTALL_BINDIR})
        endif()
    endforeach()
    if(${_exported})
        install(EXPORT _exported_targets
            FILE ${_pkg_name}_targets.cmake
            DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})
        configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
            "${CMAKE_CURRENT_SOURCE_DIR}/${_pkg_name}-config.cmake"
            INSTALL_DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
endfunction()
