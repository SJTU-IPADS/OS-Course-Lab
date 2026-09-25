# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

# For each porting method, the dir structure of its directory is mirrored
# from the structure of musl libc
# _porting_dir: dir containing files of ONE porting method(e.g. ../porting/overrides)
# _target_dir: top level dir of a target musl libc

set(_do_override_sh ${CMAKE_CURRENT_LIST_DIR}/do_override_dir.sh)
set(_do_patch_sh ${CMAKE_CURRENT_LIST_DIR}/do_patch_dir.sh)

function(_execute_ensure_success _error_msg)
    execute_process(
        ${ARGN}
        RESULT_VARIABLE _exec_result
        )
    
    if (NOT _exec_result STREQUAL "0")
        message(STATUS "${ARGN}")
        message(STATUS "Exit code: ${_exec_result}")
        message(FATAL_ERROR ${_error_msg})
    endif()
endfunction()

# based on mirrored dir strucrue of porting source dir
function(do_overrides _porting_source_dir _target_source_dir)
    _execute_ensure_success(
            "Overrides failed!"
            COMMAND bash ${_do_override_sh} ${_porting_source_dir} ${_target_source_dir}
        )
endfunction()

# based on mirrored dir strucrue of porting source dir
function(do_patches _porting_source_dir _target_source_dir)
    _execute_ensure_success(
            "Patches failed!"
            COMMAND bash ${_do_patch_sh} ${_porting_source_dir} ${_target_source_dir}
        )
endfunction()