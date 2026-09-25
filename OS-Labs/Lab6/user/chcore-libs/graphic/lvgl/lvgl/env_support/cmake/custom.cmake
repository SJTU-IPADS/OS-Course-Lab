# Option to define LV_LVGL_H_INCLUDE_SIMPLE, default: ON
option(LV_LVGL_H_INCLUDE_SIMPLE
       "Use #include \"lvgl.h\" instead of #include \"../../lvgl.h\"" ON)

# Option to define LV_CONF_INCLUDE_SIMPLE, default: ON
option(LV_CONF_INCLUDE_SIMPLE
       "Simple include of \"lv_conf.h\" and \"lv_drv_conf.h\"" ON)

# Option to set LV_CONF_PATH, if set parent path LV_CONF_DIR is added to
# includes
option(LV_CONF_PATH "Path defined for lv_conf.h")
get_filename_component(LV_CONF_DIR ${LV_CONF_PATH} DIRECTORY)

# Option to build shared libraries (as opposed to static), default: OFF
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

file(GLOB_RECURSE SOURCES ${LVGL_ROOT_DIR}/src/*.c)


if (BUILD_SHARED_LIBS)
  add_library(lvgl SHARED ${SOURCES})
else()
  add_library(lvgl STATIC ${SOURCES})
endif()

add_library(lvgl::lvgl ALIAS lvgl)


target_compile_definitions(
  lvgl PUBLIC $<$<BOOL:${LV_LVGL_H_INCLUDE_SIMPLE}>:LV_LVGL_H_INCLUDE_SIMPLE>
              $<$<BOOL:${LV_CONF_INCLUDE_SIMPLE}>:LV_CONF_INCLUDE_SIMPLE>)

# Include root and optional parent path of LV_CONF_PATH

target_include_directories(lvgl PUBLIC 
    $<BUILD_INTERFACE:${LV_CONF_DIR}> 
    $<BUILD_INTERFACE:${LV_CONF_DIR}/lvgl> 
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> # <prefix>/include 
)