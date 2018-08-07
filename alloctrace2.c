/*
 * Compile with:
 *   gcc -shared -fPIC alloctrace2.c -o alloctrace2.so -ldl
 *
 * Use with:
 *   LD_PRELOAD=./alloctrace2.so MALLOC_TRACE=logfile command [args...]
 */
#define _GNU_SOURCE
#include <mcheck.h>

static __attribute__((constructor))
void
init(void)
{
    mtrace();
}
