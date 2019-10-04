file (GLOB client_src "*.c")
add_executable (client $<TARGET_OBJECTS:libls> ${client_src})
target_link_libraries (client pthread)
target_compile_definitions (client PUBLIC -D_POSIX_C_SOURCE=200809L)
