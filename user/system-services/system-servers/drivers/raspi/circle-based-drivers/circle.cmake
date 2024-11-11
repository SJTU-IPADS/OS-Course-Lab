if(CHCORE_PLAT MATCHES "^raspi")
    string(REGEX MATCH "3|4$" _circle_rasppi "${CHCORE_PLAT}")
else()
    message(SEND_ERROR "Circle only supports Raspberry Pi series")
endif()

set(circle_home ${CMAKE_CURRENT_LIST_DIR}/../circle)
set(circle_make_options CC=${CMAKE_C_COMPILER} AR=${CMAKE_AR}
                        CIRCLEHOME=${circle_home} RASPPI=${_circle_rasppi})

chcore_get_nproc(_nproc)

add_custom_target(
    circle
    WORKING_DIRECTORY ${circle_home}
    COMMAND ./makeall ${circle_make_options} -j${_nproc})

add_custom_target(
    circle-clean
    WORKING_DIRECTORY ${circle_home}
    COMMAND ./makeall clean)

add_custom_target(circle-based-drivers-clean)

add_dependencies(system-servers-clean circle-clean circle-based-drivers-clean)

function(add_circle_target _target)
    add_custom_target(
        ${_target} ALL
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND make ${circle_make_options} ${_target}
        COMMAND mv ${_target} ${CMAKE_CURRENT_BINARY_DIR})
    add_dependencies(${_target} circle)

    add_custom_target(
        ${_target}-clean
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND make clean)
    
    add_dependencies(circle-based-drivers-clean ${_target}-clean)
endfunction(add_circle_target)

function (add_circle_lib _target)
    set(_src)
    foreach(arg ${ARGN})
        list(APPEND _src ${arg})
    endforeach()
    message("--------src: ${_src}")
    add_library(${_target} STATIC ${_src})
    add_dependencies(${_target} circle)
    target_link_libraries(${_target} PUBLIC ${circle_home}/lib/libcircle.a)
    target_include_directories(${_target} PUBLIC ${circle_home}/include)
endfunction(add_circle_lib)

# Set your circle home directory
set(CIRCLEHOME ${circle_home})

# Set your architecture and raspberry pi version
set(AARCH 64)
set(RASPPI ${_circle_rasppi})
set(STDLIB_SUPPORT 1)
set(GC_SECTIONS 0)

# Set your architecture flags
if(${AARCH} EQUAL 64)
    if(${RASPPI} EQUAL 3)
        add_definitions(-DAARCH=64)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a53 -mlittle-endian -mcmodel=small")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a53 -mlittle-endian -mcmodel=small")
    elseif(${RASPPI} EQUAL 4)
        add_definitions(-DAARCH=64)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a72 -mlittle-endian -mcmodel=small")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a72 -mlittle-endian -mcmodel=small")
    else()
        message(FATAL_ERROR "RASPPI must be set to 3 or 4")
    endif()
else()
    message(FATAL_ERROR "AARCH must be set to 64")
endif()

if(${STDLIB_SUPPORT} EQUAL 3)
    # Get the file name of libstdc++.a and libgcc_eh.a
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${ARCH} -print-file-name=libstdc++.a OUTPUT_VARIABLE LIBSTDCPP)
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${ARCH} -print-file-name=libgcc_eh.a OUTPUT_VARIABLE LIBGCC_EH)

    # Add them to the extra libs
    list(APPEND EXTRALIBS ${LIBSTDCPP} ${LIBGCC_EH})

    if(${AARCH} EQUAL 64)
        # Get the file name of crtbegin.o and crtend.o
        execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${ARCH} -print-file-name=crtbegin.o OUTPUT_VARIABLE CRTBEGIN)
        execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${ARCH} -print-file-name=crtend.o OUTPUT_VARIABLE CRTEND)
    endif()
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti -nostdinc++")
endif()

if(${STDLIB_SUPPORT} EQUAL 0)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdinc")
else()
    # Get the file name of libgcc.a
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${ARCH} -print-file-name=libgcc.a OUTPUT_VARIABLE LIBGCC)

    # Add it to the extra libs
    list(APPEND EXTRALIBS ${LIBGCC})
endif()

if(${STDLIB_SUPPORT} EQUAL 1)
    # Get the file name of libm.a
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${ARCH} -print-file-name=libm.a OUTPUT_VARIABLE LIBM)

    # Add it to the extra libs if it exists
    if(NOT ${LIBM} STREQUAL "libm.a")
        list(APPEND EXTRALIBS ${LIBM})
    endif()
endif()

# Set your garbage collection on sections
if(${GC_SECTIONS} EQUAL 1)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --gc-sections")
endif()

# Set your include directories
include_directories(
    ${CIRCLEHOME}/include/chcore
    ${CIRCLEHOME}/include
    ${CIRCLEHOME}/addon
    ${CIRCLEHOME}/addon/vc4
    ${CIRCLEHOME}/addon/vc4/interface/khronos/include
)

# Set your definitions
add_definitions(
    -DCHCORE
    -D__circle__
    -DRASPPI=${RASPPI}
    -DSTDLIB_SUPPORT=${STDLIB_SUPPORT}
    -D__VCCOREVER__=0x04000000
    -U__unix__
    -U__linux__
)

# Set your flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fsigned-char -ffreestanding -O2 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fsigned-char -ffreestanding -O2 -g")
