# Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
# Licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.

add_custom_target(system-servers-clean)
add_dependencies(system-services-clean system-servers-clean)
set(CONFIGURABLE_SERVER_CONFIG_INTERMEDIATE_FILE ${CMAKE_CURRENT_BINARY_DIR}/procmgr/configurable_servers.txt)
set(USER_SERVER_CONFIG_INTERMEDIATE_FILE ${CMAKE_CURRENT_BINARY_DIR}/procmgr/user_servers.txt)

set(CONFIGURABLE_SERVER_CONFIG_FILES "" CACHE INTERNAL "Configurable server config files" FORCE)
set(USER_SERVER_CONFIG_FILES "" CACHE INTERNAL "User server config files" FORCE)

function(chcore_add_configurable_server file)
    set(_list ${CONFIGURABLE_SERVER_CONFIG_FILES})
    list(APPEND _list ${CMAKE_CURRENT_SOURCE_DIR}/${file})
    set(CONFIGURABLE_SERVER_CONFIG_FILES ${_list} CACHE INTERNAL "Configurable server config files")
endfunction()

function(chcore_add_user_server file)
    set(_list ${USER_SERVER_CONFIG_FILES})
    list(APPEND _list ${CMAKE_CURRENT_SOURCE_DIR}/${file})
    set(USER_SERVER_CONFIG_FILES ${_list} CACHE INTERNAL "User server config files")
endfunction()

# Process Manager
add_subdirectory(procmgr)

# Shell
if(CHCORE_APP_SHELL)
    add_subdirectory(chcore_shell)
endif()

# File System Manager
add_subdirectory(fsm)

# File Systems
add_subdirectory(fs_base)
add_subdirectory(tmpfs)
if(CHCORE_FS_FAT32)
    add_subdirectory(fat32)
endif()
if(CHCORE_FS_EXT4)
    add_subdirectory(ext4)
endif()
if (CHCORE_FS_LITTLEFS)
    add_subdirectory(littlefs)
endif()

# Network Stack
if(CHCORE_SERVER_LWIP)
    add_subdirectory(lwip)
endif()

# Device Drivers
add_subdirectory(drivers)

# GUI
if(CHCORE_SERVER_GUI)
    add_subdirectory(gui)
    add_subdirectory(terminal)
endif()

# SHM POSIX API
if(CHCORE_SERVER_POSIX_SHM)
    add_subdirectory(posix_shm)
endif()

if(CHCORE_DAEMON_NETCP)
    add_subdirectory(daemon-services/network-cp)
endif()

if(CHCORE_SERVER_GDB)
    add_subdirectory(gdbserver)
endif()

if (CHCORE_MINI)
    chcore_all_force_static_linked()
else()
    chcore_all_dynamic_linked_exec()
endif()

add_custom_target(generate_configurable_server_intermediate_file
    COMMAND ${CMAKE_COMMAND} -E echo "${CONFIGURABLE_SERVER_CONFIG_FILES}" > ${CONFIGURABLE_SERVER_CONFIG_INTERMEDIATE_FILE}
    COMMAND ${CMAKE_COMMAND} -E echo "CONFIG_FILE_PATH=\"${CONFIGURABLE_SERVER_CONFIG_INTERMEDIATE_FILE}\""
    COMMENT "Generating config file"
    VERBATIM
)

add_custom_target(generate_user_server_intermediate_file
    COMMAND ${CMAKE_COMMAND} -E echo "${USER_SERVER_CONFIG_FILES}" > ${USER_SERVER_CONFIG_INTERMEDIATE_FILE}
    COMMAND ${CMAKE_COMMAND} -E echo "USER_SERVER_CONFIG_FILE_PATH=\"${USER_SERVER_CONFIG_INTERMEDIATE_FILE}\""
    COMMENT "Generating user server config file"
    VERBATIM
)

add_dependencies(generate_configure_file generate_configurable_server_intermediate_file)
add_dependencies(generate_configure_file generate_user_server_intermediate_file)