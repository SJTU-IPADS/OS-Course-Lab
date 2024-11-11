add_library(audio_driver audio_driver.c)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/audio_driver.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})