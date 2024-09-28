# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

set(SYS_SERVER_CONFIGURE_FILE "/server_configure.yaml")
include_directories(include)
add_subdirectory(libs)
add_executable(
    procmgr.srv
    proc_node.c
    procmgr.c
    recycle.c
    shell_msg_handler.c
    start_daemon_service.c
    srvmgr.c
    loader.c)

add_library(cyaml STATIC IMPORTED)
set_target_properties(cyaml PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libs/libcyaml.a")
add_library(yaml STATIC IMPORTED)
set_target_properties(yaml PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libs/libyaml.a")
target_link_libraries(procmgr.srv PRIVATE chcoreelf)
target_link_libraries(procmgr.srv PRIVATE launch)
target_link_libraries(procmgr.srv PRIVATE refcnt)
target_link_libraries(procmgr.srv PRIVATE cyaml)
target_link_libraries(procmgr.srv PRIVATE yaml)
target_compile_definitions(procmgr.srv PRIVATE SYS_SERVER_CONFIGURE_FILE="${SYS_SERVER_CONFIGURE_FILE}")

macro(_procmgr_incbin _target_name _binary_name _binary_path)
    set(_binary_name ${_binary_name})
    set(_binary_path ${_binary_path})
    configure_file(incbin_basic_srv.tpl.S incbin_${_binary_name}.S)
    target_sources(procmgr.srv PRIVATE incbin_${_binary_name}.S)
    unset(_binary_name)
    unset(_binary_path)
endmacro()

_procmgr_incbin(fsm.srv fsm_elf
                ${CHCORE_PROJECT_DIR}/ramdisk/fsm.srv)
_procmgr_incbin(tmpfs.srv tmpfs_elf
                ${CHCORE_PROJECT_DIR}/ramdisk/tmpfs.srv)

add_custom_target(touch-inc-tmpfs
                COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/incbin_tmpfs_elf.S)

add_dependencies(procmgr.srv touch-inc-tmpfs)

target_link_options(procmgr.srv PRIVATE -T
                    ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -z
                    max-page-size=4096)

add_custom_target(
    create-procmgr-bin ALL
    COMMAND echo "build procmgr binary..."
    COMMAND read_procmgr_elf_tool ${CMAKE_CURRENT_BINARY_DIR}/procmgr.srv
    COMMAND chmod 666 elf_info.temp
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/procmgr.srv procmgr
    COMMAND exec 3<>procmgr && 
            cat elf_info.temp >&3
    COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/elf_info.temp
    COMMAND echo "build procmgr success!"
    DEPENDS procmgr.srv)

chcore_all_force_static_linked()

get_directory_property(defs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS)
add_custom_target(
    generate_configure_file ALL
    COMMAND python3 
    ${CMAKE_CURRENT_SOURCE_DIR}/srvmgr_config_generator/generate_config.py 
    ${CONFIGURABLE_SERVER_CONFIG_INTERMEDIATE_FILE}
    ${USER_SERVER_CONFIG_INTERMEDIATE_FILE}
    ${CHCORE_RAMDISK_DIR}/${SYS_SERVER_CONFIGURE_FILE} ARGS ${defs}
)

