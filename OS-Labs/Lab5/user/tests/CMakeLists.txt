cmake_minimum_required(VERSION 3.14)
project(ChCoreTests ASM C)
add_subdirectory(fs_tests)

include(CommonTools)
include(LibAppTools)

chcore_copy_all_targets_to_ramdisk()
