file (GLOB libls_src "*.c")
add_library (libls OBJECT ${libls_src})
target_compile_definitions (libls PUBLIC -D_POSIX_C_SOURCE=200809L)
