add_library(guilog SHARED guidebug.c savebmp.c)
target_link_libraries(guilog PUBLIC graphic)
target_compile_options(guilog PRIVATE -Wno-error)
