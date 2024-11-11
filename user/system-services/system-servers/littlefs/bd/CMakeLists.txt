add_library(bd bd_${CHCORE_FS_LITTLEFS_BD}.c)

target_compile_definitions(bd PRIVATE LFS_THREADSAFE)

target_link_libraries(bd PRIVATE pthread)