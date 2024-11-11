if(ESP_PLATFORM)

    file(GLOB_RECURSE SOURCES src/*.c test/*.c)

    idf_build_get_property(LV_MICROPYTHON LV_MICROPYTHON)

    if (LV_MICROPYTHON)
        idf_component_register(SRCS ${SOURCES}
                               INCLUDE_DIRS . src test ../
                               REQUIRES lv_lib_100ask)
    else()
        idf_component_register(SRCS ${SOURCES}
                               INCLUDE_DIRS . src test
                               REQUIRES lv_lib_100ask)
    endif()

    target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_LIB_100ASK_ESP_PLATFORM")

else()

    file(GLOB_RECURSE SOURCES src/*.c test/*.c)
    add_library(lv_lib_100ask STATIC ${SOURCES})
    target_compile_definitions(lv_lib_100ask PUBLIC "-DLV_LVGL_H_INCLUDE_SIMPLE")
    target_link_libraries(lv_lib_100ask PRIVATE lvgl lv_drivers)
    target_compile_options(lv_lib_100ask PRIVATE -Wno-error)


    add_executable(lv_memgame.bin lv_memgame.c)
    target_link_libraries(lv_memgame.bin PUBLIC lv_lib_100ask lvgl lv_drivers)

    add_executable(lv_cal.bin lv_cal.c)
    target_link_libraries(lv_cal.bin PUBLIC lv_lib_100ask lvgl lv_drivers)

    add_executable(lv_file.bin lv_file.c)
    target_link_libraries(lv_file.bin PUBLIC lv_lib_100ask lvgl lv_drivers)

    add_executable(lv_2048.bin lv_2048.c)
    target_link_libraries(lv_2048.bin PUBLIC lv_lib_100ask lvgl lv_drivers)
endif()

