# 递归查找文件夹及其子目录下的所有C文件
file(GLOB_RECURSE SOURCES "*.c")

# 编译为可执行文件
add_executable(lv_text_editor.bin ${SOURCES})

# 链接lvgl和lv_drivers库
target_link_libraries(lv_text_editor.bin
    lvgl
    lv_drivers
)
# 关闭-Werror编译选项
target_compile_options(lv_text_editor.bin PRIVATE "-Wno-error")