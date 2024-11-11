add_compile_options(
    -fPIC
    -fstack-protector-all 
    -D_FORTIFY_SOURCE=2 
    -O2 
    -Wformat=2 
    -Wfloat-equal 
    -Wshadow 
    -Wconversion 
    -Wformat-security 
    -Wextra 
    --param ssp-buffer-size=4 
    -Warray-bounds 
    -Wpointer-arith 
    -Wcast-qual 
    -Wstrict-prototypes 
    -Wmissing-prototypes 
    -Wstrict-overflow=1 
    -Wstrict-aliasing=2 
    -Wswitch 
    -Wswitch-default 
    -Wall 
    -DNDEBUG 
    -O2
    )

add_link_options(-fPIC -s -Wl,-z,relro,-z,now,-z,noexecstack)

file(GLOB bounds_check_source_files "libboundscheck/src/*.c")
add_library(boundscheck SHARED ${bounds_check_source_files})
add_library(boundscheck_static STATIC ${bounds_check_source_files})
chcore_target_include_and_export_directory(boundscheck PUBLIC libboundscheck/include)
chcore_target_include_and_export_directory(boundscheck_static PUBLIC libboundscheck/include)