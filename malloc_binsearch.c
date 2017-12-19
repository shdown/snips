#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main() {
    if ((malloc(SIZE_MAX))) {
        fprintf(stderr, "malloc never fails!\n");
        return 0;
    }
    size_t lb = 1, rb = SIZE_MAX;
    while (rb - lb > 1) {
        size_t m = lb + (rb - lb) / 2;
        fprintf(stderr, "mallocing %zu... ", m);
        void *p = malloc(m);
        fprintf(stderr, "%p\n", p);
        *(p ? &lb : &rb) = m;
        free(p);
    }
    ++lb;
    fprintf(stderr, "malloc starts failing at %zu, which is %.2f Gb.\n", lb, lb / 1024. / 1024. / 1024.);
    return 0;
}
