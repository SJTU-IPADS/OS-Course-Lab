include_directories(${CMAKE_CURRENT_LIST_DIR} littlefs)
add_library(lfs littlefs/bd/lfs_rambd.c littlefs/lfs.c littlefs/lfs_util.c littlefs/lfs.h littlefs/lfs_util.h)
target_compile_definitions(lfs PRIVATE LFS_THREADSAFE)

add_subdirectory(bd)

if(CHCORE_MINI)
    add_user_executable_static_linked(littlefs.srv chcore_littlefs_server.c chcore_littlefs_ops.c chcore_littlefs_defs.h)
else()
    add_user_executable(littlefs.srv chcore_littlefs_server.c chcore_littlefs_ops.c chcore_littlefs_defs.h)
endif()
target_compile_definitions(littlefs.srv PRIVATE LFS_THREADSAFE)
target_link_libraries(littlefs.srv PRIVATE fs_base lfs pthread bd)

chcore_copy_all_targets_to_ramdisk()
