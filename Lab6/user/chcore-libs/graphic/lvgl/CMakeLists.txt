cmake_minimum_required(VERSION 3.14)

# Specify path to own LVGL config header
set(LV_CONF_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h
    CACHE STRING "" FORCE)

# Fetch LVGL from GitHub
set(PKG_WAYLAND_LIBRARIES wayland-client wayland-cursor)
add_subdirectory(lvgl)
add_subdirectory(lv_drivers)

function(install_headers_recursively dir)
    file(GLOB children LIST_DIRECTORIES true "${dir}/*")
    foreach(child ${children})
        if(IS_DIRECTORY ${child})
            install_headers_recursively(${child})
        else()
            get_filename_component(ext ${child} EXT)
            if("${ext}" STREQUAL ".h")
                file(RELATIVE_PATH relpath ${CMAKE_CURRENT_SOURCE_DIR} ${child})
                get_filename_component(dirname ${relpath} DIRECTORY)
                install(FILES ${child} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${dirname})
            endif()
        endif()
    endforeach()
endfunction()

install_headers_recursively(${CMAKE_CURRENT_SOURCE_DIR}/lvgl)
install_headers_recursively(${CMAKE_CURRENT_SOURCE_DIR}/lv_drivers)

# Install .h files in the current directory
file(GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lvgl/lvgl.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
 