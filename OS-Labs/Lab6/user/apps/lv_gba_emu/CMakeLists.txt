cmake_minimum_required(VERSION 3.10)
project(gba_emu)

# C/C++ version
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# Platform config

set(ENABLE_ASAN 0)
# set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_BUILD_TYPE "Release")

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")

## Common
# Platform

message(STATUS "Platform: UNIX - Wayland")
add_definitions(
        -DLV_SCREEN_HOR_RES=320
        -DLV_SCREEN_VER_RES=480
        -DTHREADED_RENDERER=0
)

add_definitions(-DHAVE_UNISTD_H=1 -DHAVE_FCNTL_H=1)

# Address Sanitizer
if (ENABLE_ASAN)
        set(ASAN_FLAGS "-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
        add_definitions(${ASAN_FLAGS})
        set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} ${ASAN_FLAGS})
endif()

# LVGL config
add_definitions(
        -DLV_USE_DEV_VERSION=1
)

# vba-next config
add_definitions(
        -DDEBUG=0
        -DFRONTEND_SUPPORTS_RGB565=1
        -DTILED_RENDERING=1
        -DSTATIC_LINKING=0
        -DUSE_TWEAKS=1
        -DUSE_FRAME_SKIP=1
        -DHAVE_NEON=0
        -Dretro_sleep=lv_port_sleep

        -DNDEBUG
        -D__LIBRETRO__
        -DINLINE=inline
        -DHAVE_STDINT_H
        -DHAVE_INTTYPES_H

        -DGBA_FRAME_SKIP="0"
)

# Include
set(VBA_NEXT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/vba-next)

include_directories(
        ${PROJECT_SOURCE_DIR}
        ${VBA_NEXT_SOURCE_DIR}/src
        ${VBA_NEXT_SOURCE_DIR}/libretro-common/include
)

# Sources
## porting
file(GLOB PORT_SOURCES "port/*.c")

## gba emu
file(GLOB GBA_EMU_SOURCES "gba_emu/*.c*")

## vba-next
file(GLOB VBA_NEXT_SOURCE
        ${VBA_NEXT_SOURCE_DIR}/src/*.c
        ${VBA_NEXT_SOURCE_DIR}/src/*.cpp
        ${VBA_NEXT_SOURCE_DIR}/libretro/libretro.cpp
        ${VBA_NEXT_SOURCE_DIR}/libretro-common/rthreads/rsemaphore.c
        ${VBA_NEXT_SOURCE_DIR}/libretro-common/rthreads/rthreads.c
        ${VBA_NEXT_SOURCE_DIR}/libretro-common/compat/*.c
        ${VBA_NEXT_SOURCE_DIR}/libretro-common/encodings/*.c
        ${VBA_NEXT_SOURCE_DIR}/libretro-common/file/*.c
        ${VBA_NEXT_SOURCE_DIR}/libretro-common/streams/*.c
        ${VBA_NEXT_SOURCE_DIR}/libretro-common/string/*.c
        ${VBA_NEXT_SOURCE_DIR}/libretro-common/time/*.c
)

## ALL SOURCES
set(SOURCES
        ${PORT_SOURCES}
        ${GBA_EMU_SOURCES}
        ${GBA_PORT_SOURCES}
        ${VBA_NEXT_SOURCE}
)


# Executable
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_executable(gba_emu.bin main.c ${SOURCES} ${INCLUDES})
target_compile_options(gba_emu.bin PRIVATE -Wno-error)
# Link
target_link_libraries(
        gba_emu.bin PRIVATE
        lvgl
        pthread
        # lvgl::examples
        # lvgl::demos
        ${RPI_LINK_LIBS}
        ${SDL2_LIBRARIES}
        lvgl
        lv_drivers
        audio_driver
)