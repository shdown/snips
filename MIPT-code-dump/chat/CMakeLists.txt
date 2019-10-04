cmake_minimum_required (VERSION 3.0.0)
project (task2 C)

#------------------------------------------------------------------------------

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
endif ()

#------------------------------------------------------------------------------

set (CMAKE_C_STANDARD 99)

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"
)
    if (${CMAKE_BUILD_TYPE} MATCHES "Release|MinSizeRel")
        set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
    endif ()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall -Wextra -Wcast-qual -Wno-cast-align -Wfloat-equal -Wredundant-decls -Wshadow -Wpointer-arith -Winit-self")
    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wlogical-op")
    endif ()
endif ()

include_directories (.)

add_subdirectory (libls)
add_subdirectory (server)
add_subdirectory (client)
add_subdirectory (tester)
