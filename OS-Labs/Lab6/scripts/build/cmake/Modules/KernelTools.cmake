# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

# Add source files to target with specified scope, and output an object list.
macro(chcore_target_sources_out_objects _target _scope _objects)
    target_sources(${_target} ${_scope} ${ARGN})

    if(NOT CMAKE_CURRENT_BINARY_DIR MATCHES "^${CMAKE_BINARY_DIR}/")
        message(
            FATAL_ERROR
                "CMAKE_CURRENT_BINARY_DIR (${CMAKE_BINARY_DIR}) must be in CMAKE_BINARY_DIR (${CMAKE_BINARY_DIR})."
        )
    endif()
    string(REGEX REPLACE "^${CMAKE_BINARY_DIR}/" "" _curr_bin_rel_path
                         ${CMAKE_CURRENT_BINARY_DIR})

    foreach(_src ${ARGN})
        if(_src MATCHES "\.(c|C)$")
            set(_obj_extension ${CMAKE_C_OUTPUT_EXTENSION})
        elseif(_src MATCHES "\.(s|S)$")
            set(_obj_extension ${CMAKE_ASM_OUTPUT_EXTENSION})
        elseif(_src MATCHES "\.(cpp|CPP|cxx|CXX|cc|CC)$")
            set(_obj_extension ${CMAKE_CXX_OUTPUT_EXTENSION})
        else()
            message(FATAL_ERROR "Unsupported file type: ${_src}")
        endif()
        list(
            APPEND
            ${_objects}
            CMakeFiles/${_target}.dir/${_curr_bin_rel_path}/${_src}${_obj_extension}
        )
    endforeach()

    unset(_obj_extension)
    unset(_curr_bin_rel_path)
endmacro()

# Add target to convert ELF kernel to binary image.
function(chcore_objcopy_binary _kernel_target _binary_name)
    add_custom_target(
        ${_binary_name} ALL
        COMMAND ${CMAKE_OBJCOPY} -O binary -S $<TARGET_FILE:${_kernel_target}>
                ${_binary_name}
        DEPENDS ${_kernel_target})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${_binary_name}
            DESTINATION ${CMAKE_INSTALL_PREFIX})
endfunction()

# Add target to generate qemu emulation script.
function(chcore_generate_emulate_sh _qemu _qemu_options)
    set(qemu ${_qemu})
    set(qemu_options ${_qemu_options})
    configure_file(${CHCORE_PROJECT_DIR}/scripts/qemu/emulate.tpl.sh emulate.sh
                   @ONLY)
    unset(qemu)
    unset(qemu_options)

    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/emulate.sh
            DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(
        PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/emulate.sh
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        RENAME simulate.sh)
endfunction()
