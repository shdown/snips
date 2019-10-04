file (GLOB tester_src "*.c")
add_executable (tester $<TARGET_OBJECTS:libls> ${tester_src})
target_compile_definitions (client PUBLIC -D_POSIX_C_SOURCE=200809L)
