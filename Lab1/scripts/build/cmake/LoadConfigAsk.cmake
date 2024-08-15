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
Load config values from `.config` and check if all
cache variables defined in `config.cmake` are set,
if not, ask user interactively.

This script is intended to be used as -C option of
cmake command.
#]]

set(_input_sh ${CMAKE_CURRENT_LIST_DIR}/Helpers/input.sh)

# Ask user for input
function(_ask_for_input _prompt _result)
    execute_process(
        COMMAND bash ${_input_sh} ${_prompt}
        ERROR_VARIABLE _tmp
        ERROR_STRIP_TRAILING_WHITESPACE)
    set(${_result}
        ${_tmp}
        PARENT_SCOPE)
endfunction()

# Ask user for yes or no
function(_ask_for_yn _prompt _default _yn_var)
    while(1)
        _ask_for_input(${_prompt} _result)
        if("${_result}" MATCHES "^(y|Y)")
            set(${_yn_var}
                "y"
                PARENT_SCOPE)
        elseif("${_result}" MATCHES "^(n|N)")
            set(${_yn_var}
                "n"
                PARENT_SCOPE)
        elseif("${_result}" STREQUAL "")
            set(${_yn_var}
                ${_default}
                PARENT_SCOPE)
        else()
            execute_process(COMMAND echo "Invalid input!")
            continue()
        endif()
        break()
    endwhile()
endfunction()

macro(chcore_config _config_name _config_type _default _description)
    if(NOT DEFINED ${_config_name})
        _ask_for_yn(
            "${_config_name} is not set, use default (${_default})? (Y/n)" "y"
            _answer)
        set(_value ${_default})
        if(NOT "${_answer}" STREQUAL "y")
            _ask_for_input("Enter a value for ${_config_name}:" _value)
        endif()
        set(${_config_name}
            ${_value}
            CACHE ${_config_type} ${_description} FORCE)
    endif()
endmacro()

include(${CMAKE_CURRENT_LIST_DIR}/LoadConfig.cmake)
