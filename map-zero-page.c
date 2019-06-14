// Compile with:
//      cc -O0 map-zero-page.c -o map-zero-page
// When run as non-root:
//      mmap: Operation not permitted
//      Segmentation fault
// When run as root:
//      (nil)
//      x = 0

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>

int main() {
    void *ptr = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
    } else {
        printf("%p\n", ptr);
    }
    int x = *(int *) NULL;
    printf("x = %d\n", x);
}
