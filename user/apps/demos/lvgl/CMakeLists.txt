add_executable(lvgl_demo.bin demo/demo.c)
add_executable(llllllll.bin tetris/main.c tetris/tetris.c tetris/display.c tetris/tetriminos.c tetris/style.c)
file(GLOB_RECURSE EXAMPLE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.c)
file(GLOB_RECURSE DEMO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/demos/*.c)
add_library(lvgl_examples STATIC ${EXAMPLE_SOURCES})
add_library(lvgl::examples ALIAS lvgl_examples)
add_library(lvgl_demos STATIC ${DEMO_SOURCES})
add_library(lvgl::demos ALIAS lvgl_demos)
# Include /examples folder
target_include_directories(lvgl_examples PUBLIC examples)
target_include_directories(lvgl_demos PUBLIC demos)
target_link_libraries(lvgl_examples PUBLIC lvgl)
target_link_libraries(lvgl_demos PUBLIC lvgl)


target_link_libraries(lvgl_demo.bin PRIVATE lvgl lv_drivers lvgl_demos wayland-client)
target_compile_options(lvgl_demo.bin PRIVATE -Wno-error)

target_link_libraries(llllllll.bin PRIVATE lvgl lv_drivers wayland-client)
target_compile_options(llllllll.bin PRIVATE -Wno-error)

add_subdirectory(lv_lib_100ask)
add_subdirectory(lv_text_editor)

add_subdirectory(lv_gba_emu)
