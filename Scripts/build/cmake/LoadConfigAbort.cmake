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
if not, abort.

This script is intended to be used as -C option of
cmake command.
#]]

macro(chcore_config _config_name _config_type _default _description)
    if(NOT DEFINED ${_config_name})
        message(FATAL_ERROR "${_config_name} is not set")
    endif()
endmacro()

include(${CMAKE_CURRENT_LIST_DIR}/LoadConfig.cmake)
