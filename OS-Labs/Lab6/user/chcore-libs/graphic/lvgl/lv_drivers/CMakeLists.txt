cmake_minimum_required(VERSION 3.12.4)

project(lv_drivers HOMEPAGE_URL https://github.com/lvgl/lv_drivers/)

# Option to build as shared library (as opposed to static), default: OFF
option(BUILD_SHARED_LIBS "Build shared as library (as opposed to static)" ON)

file(GLOB_RECURSE SOURCES ./*.c)

if(BUILD_SHARED_LIBS)
    add_library(lv_drivers SHARED ${SOURCES})
else()
    add_library(lv_drivers STATIC ${SOURCES})
endif()

add_library(lvgl_drivers ALIAS lv_drivers)
add_library(lvgl::drivers ALIAS lv_drivers)

find_package(PkgConfig)
pkg_check_modules(PKG_WAYLAND wayland-client wayland-cursor wayland-protocols
                  xkbcommon)
pkg_check_modules(PKG_LVGL lvgl)
target_link_libraries(lv_drivers PUBLIC lvgl ${PKG_WAYLAND_LIBRARIES})
