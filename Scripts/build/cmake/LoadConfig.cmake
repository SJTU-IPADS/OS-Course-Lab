# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

#[[
Load cache variables from `.config` and `config.cmake`.

This script is intended to be used as -C option of
cmake command.
#]]

include(${CMAKE_CURRENT_LIST_DIR}/Modules/CommonTools.cmake)

if(EXISTS ${CMAKE_SOURCE_DIR}/.config)
    # Read in config file
    file(READ ${CMAKE_SOURCE_DIR}/.config _config_str)
    string(REPLACE "\n" ";" _config_lines "${_config_str}")
    unset(_config_str)

    # Set config cache variables
    foreach(_line ${_config_lines})
        if(${_line} MATCHES "^//" OR ${_line} MATCHES "^#")
            continue()
        endif()
        string(REGEX MATCHALL "^([^:=]+):([^:=]+)=(.*)$" _config "${_line}")
        if("${_config}" STREQUAL "")
            message(FATAL_ERROR "Invalid line in `.config`: ${_line}")
        endif()
        set(${CMAKE_MATCH_1}
            ${CMAKE_MATCH_3}
            CACHE ${CMAKE_MATCH_2} "" FORCE)
    endforeach()
    unset(_config_lines)
else()
    message(WARNING "There is no `.config` file")
endif()

# Check if there exists `chcore_config` macro, which will be used in
# `config.cmake`
if(NOT COMMAND chcore_config)
    message(FATAL_ERROR "Don't directly use `LoadConfig.cmake`")
endif()

macro(chcore_config _config_name _config_type _default _description)
    if(DEFINED ${_config_name})
        # config is in `.config`, set description
        set(${_config_name}
            ${${_config_name}}
            CACHE ${_config_type} ${_description} FORCE)
    else()
        # config is not in `.config`, use previously-defined chcore_config
        # Note: use quota marks to allow forwarding empty arguments
        _chcore_config("${_config_name}" "${_config_type}" "${_default}"
                       "${_description}")
    endif()
endmacro()

# Include the top-level config definition file
include(${CMAKE_SOURCE_DIR}/config.cmake)

# Hide unrelavant builtin cache variables
mark_as_advanced(CMAKE_BUILD_TYPE)
mark_as_advanced(CMAKE_INSTALL_PREFIX)
