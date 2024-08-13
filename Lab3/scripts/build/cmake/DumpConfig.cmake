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
Dump cache variables from `CMakeCache.txt` to `.config`.

This script is intended to be used as -C option of
cmake command.
#]]

include(${CMAKE_CURRENT_LIST_DIR}/Modules/CommonTools.cmake)

set(_config_lines)

macro(chcore_config _config_name _config_type _default _description)
    # Dump config lines in definition order
    list(APPEND _config_lines
         "${_config_name}:${_config_type}=${${_config_name}}")
endmacro()

include(${CMAKE_SOURCE_DIR}/config.cmake)

string(REPLACE ";" "\n" _config_str "${_config_lines}")

file(WRITE ${CMAKE_SOURCE_DIR}/.config "${_config_str}\n")
