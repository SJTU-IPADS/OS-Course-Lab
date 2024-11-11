
set (YAML_VERSION_MAJOR 0)
set (YAML_VERSION_MINOR 2)
set (YAML_VERSION_PATCH 5)
set (YAML_VERSION_STRING "${YAML_VERSION_MAJOR}.${YAML_VERSION_MINOR}.${YAML_VERSION_PATCH}")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")

set(YAML_STATIC_LIB_NAME "yaml" CACHE STRING "Base name of static library output")

#
# Output directories for a build tree
#
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()


#
# Build library
#
set(SRCS
  src/api.c
  src/dumper.c
  src/emitter.c
  src/loader.c
  src/parser.c
  src/reader.c
  src/scanner.c
  src/writer.c
  )

set(config_h ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)
configure_file(
  cmake/config.h.in
  ${config_h}
  )

add_library(yaml ${SRCS})

if(NOT BUILD_SHARED_LIBS)
  set_target_properties(yaml
    PROPERTIES OUTPUT_NAME ${YAML_STATIC_LIB_NAME}
  )
endif()

set_target_properties(yaml
  PROPERTIES DEFINE_SYMBOL YAML_DECLARE_EXPORT
  )

target_compile_definitions(yaml
  PRIVATE HAVE_CONFIG_H
  PUBLIC
    $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:YAML_DECLARE_STATIC>
    $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>
  )

target_include_directories(yaml PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  )

#
# Install rules
#
install(
  FILES
    include/yaml.h
  DESTINATION include COMPONENT Development
  )

install(
  TARGETS yaml
  EXPORT yamlTargets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT Runtime
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT Development
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT Development
  )
