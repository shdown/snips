/*
 * Just an example of symbol overriding with LD_PRELOAD.
 *
 * Compile with:
 *   gcc -shared -fPIC alloctrace.c -o alloctrace.so -ldl
 *
 * Use with:
 *   LD_PRELOAD=./alloctrace.so command [args...]
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>

static void * (*malloc0)(size_t n);
static void * (*realloc0)(void *p, size_t n);
static void (*free0)(void *p);

static __attribute__((constructor))
void
init(void)
{
    *(void **) &malloc0 = dlsym(RTLD_NEXT, "malloc");
    *(void **) &realloc0 = dlsym(RTLD_NEXT, "realloc");
    *(void **) &free0 = dlsym(RTLD_NEXT, "free");
}

static
int
full_write(int fd, const char *buf, size_t nbuf)
{
    for (size_t nwritten = 0; nwritten != nbuf;) {
        ssize_t w = write(fd, buf + nwritten, nbuf - nwritten);
        if (w < 0) {
            return -1;
        }
        nwritten += w;
    }
    return 0;
}

static inline
void
reverse(char *buf, size_t nbuf)
{
    for (size_t i = 0; i < nbuf / 2; ++i) {
        char tmp = buf[i];
        buf[i] = buf[nbuf - i - 1];
        buf[nbuf - i - 1] = tmp;
    }
}

static
int
full_write_zu(int fd, size_t val)
{
    char buf[256];
    size_t n;
    for (n = 0; val; ++n) {
        buf[n] = '0' + val % 10;
        val /= 10;
    }
    if (!n) {
        buf[n++] = '0';
    }
    reverse(buf, n);
    return full_write(fd, buf, n);
}

static
int
full_write_p(int fd, void *p)
{
    static const char *HEX = "0123456789abcdef";
    uintptr_t val = (uintptr_t) p;
    char buf[256];
    size_t n;
    for (n = 0; val; ++n) {
        buf[n] = HEX[val % 16];
        val /= 16;
    }
    if (n) {
        buf[n++] = 'x';
        buf[n++] = '0';
    } else {
        buf[n++] = '0';
    }
    reverse(buf, n);
    return full_write(fd, buf, n);
}

static
int
full_write_s(int fd, const char *s)
{
    size_t len;
    for (len = 0; s[len]; ++len) {}
    return full_write(fd, s, len);
}

#define S(...) full_write_s(2, __VA_ARGS__)
#define P(...) full_write_p(2, __VA_ARGS__)
#define Z(...) full_write_zu(2, __VA_ARGS__)

void *
malloc(size_t n) {
    S("@malloc("), Z(n), S(") = ");
    void *r = malloc0(n);
    P(r), S("\n");
    return r;
}

void *
realloc(void *p, size_t n) {
    S("@realloc("), P(p), S(", "), Z(n), S(") = ");
    void *r = realloc0(p, n);
    P(r), S("\n");
    return r;
}

void
free(void *p) {
    S("@free("), P(p), S(")\n");
    free0(p);
}
