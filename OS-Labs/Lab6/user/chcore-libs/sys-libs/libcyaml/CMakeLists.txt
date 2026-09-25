# 
# set(VERSION_MAJOR 2)
# set(VERSION_MINOR 0)
# set(VERSION_PATCH 0)
# set(VERSION_DEVEL 1)
# set(VERSION_STR "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_DEVEL}")

# file(GLOB cyaml_src "src/*.c")

# add_library(cyaml ${cyaml_src})
chcore_import_library(cyaml STATIC ./libcyaml.a)
target_link_libraries(cyaml INTERFACE yaml)
# target_link_libraries(cyaml PRIVATE yaml)

# target_compile_definitions(cyaml PRIVATE
#   VERSION_MAJOR=${VERSION_MAJOR}
#   VERSION_MINOR=${VERSION_MINOR}
#   VERSION_PATCH=${VERSION_PATCH}
#   VERSION_DEVEL=${VERSION_DEVEL}
#   VERSION_STR="${VERSION_STR}"
# )

chcore_target_include_and_export_directory(cyaml INTERFACE include)