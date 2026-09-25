cmake_minimum_required(VERSION 3.14)
project(ChCoreApps ASM C CXX)

find_package(chcore-libs REQUIRED)

add_subdirectory(fs_tests)
add_subdirectory(lv_lib_100ask)

include(CommonTools)
include(LibAppTools)

add_executable(wayland_demo.bin wayland_demo.c)
target_link_libraries(wayland_demo.bin PRIVATE wayland-client)

add_subdirectory(lv_gba_emu)
chcore_copy_all_targets_to_ramdisk()
