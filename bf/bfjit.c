#define _GNU_SOURCE
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>

//------------------------------------------------------------------------------

static
void *
x2realloc(void *p, size_t *pn, size_t m)
{
    if (*pn) {
        if (m && *pn > SIZE_MAX / 2 / m) {
            goto oom;
        }
        *pn *= 2;
    } else {
        *pn = 1;
    }
    if (!(p = realloc(p, *pn * m))) {
        goto oom;
    }
    return p;
oom:
    fputs("Out of memory.\n", stderr);
    abort();
}

#define VECTOR_OF(T_) \
    struct { \
        T_ *data; \
        size_t size; \
        size_t capacity; \
    }

#define VECTOR_NEW() {NULL, 0, 0}

#define VECTOR_PUSH(V_, X_) \
    do { \
        if ((V_).size == (V_).capacity) { \
            (V_).data = x2realloc((V_).data, &(V_).capacity, sizeof(*(V_).data)); \
        } \
        (V_).data[(V_).size++] = (X_); \
    } while (0)

#define VECTOR_POP(V_) (V_).data[--(V_).size]

#define VECTOR_FREE(V_) free((V_).data)

//------------------------------------------------------------------------------

static size_t size;
static size_t capacity = 4;
static unsigned char *ptr;

static
void
grow(void)
{
    const size_t new_capacity = capacity * 2;
    ptr = mremap(ptr, capacity, new_capacity, MREMAP_MAYMOVE);
    if (ptr == MAP_FAILED) {
        perror("mremap");
        abort();
    }
    capacity = new_capacity;
}

#define push(X_) \
    if (size == capacity) { \
        grow(); \
    } \
    ptr[size++] = (X_) \

static inline
void
push4(unsigned x)
{
    if (capacity - size < 4) {
        grow();
    }
    memcpy(ptr + size, &x, 4);
    size += 4;
}

static inline
void
push8(unsigned long x)
{
    if (capacity - size < 8) {
        grow();
    }
    memcpy(ptr + size, &x, 8);
    size += 8;
}

static inline
void
fixup4(size_t pos)
{
    memcpy(ptr + pos - 4, (unsigned [1]) {size - pos}, 4);
}

//------------------------------------------------------------------------------

enum {
    RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
};

enum {
    R8, R9, R10, R11, R12, R13, R14, R15
};

#define i_movq_r_im8(Reg_, X_) \
    push(0x48); push(0xB8 + (Reg_)); push8(X_)

#define i_movq_r_im(Reg_, X_) \
    push(0x48); push(0xC7); push(0xC0 + (Reg_)); push4(X_)

#define i_movq_r_r(Dst_, Src_) \
    push(0x48); push(0x89); push(0xC0 + (Dst_) + 8 * (Src_))

#define i_movq_rn_im(Reg_, X_) \
    push(0x49); push(0xC7); push(0xC0 + (Reg_)); push4(X_)

#define i_incq_r(Reg_) \
    push(0x48); push(0xFF); push(0xC0 + (Reg_))

#define i_incb_m(Reg_) \
    push(0xFE); push(Reg_)

#define i_decq_r(Reg_) \
    push(0x48); push(0xFF); push(0xC8 + (Reg_))

#define i_decb_m(Reg_) \
    push(0xFE); push(010 + (Reg_))

#define i_cmpb_m_im(Reg_, X_) \
    push(0x80); push(070 + (Reg_)); push(X_)

#define i_test_r_r(A_, B_) \
    push(0x48); push(0x85); push(0xC0 + (A_) + 8 * (B_))

#define i_negq_r(Reg_) \
    push(0x48); push(0xF7); push(0xD8 + (Reg_))

#define i_shrq_r_im(Reg_, X_) \
    push(0x48); push(0xC1); push(0xE8 + (Reg_)); push(X_)

#define i_andb_m_r(Ptr_, Reg_) \
    push(0x20); push((Ptr_) + 8 * (Reg_))

#define i_syscall() \
    push(0x0F); push(0x05)

#define i_js(Off_) \
    push(0x0F); push(0x88); push4(Off_)

#define i_je(Off_) \
    push(0x0F); push(0x84); push4(Off_)

#define i_jmp(Off_) \
    push(0xE9); push4(Off_)

#define i_call(Off_) \
    push(0xE8); push4(Off_)

#define i_ret() \
    push(0xC3)

#define i_popq_r(Reg_) \
    push(0x58 + (Reg_))

#define i_pushq_r(Reg_) \
    push(0x50 + (Reg_))

static const char *DUMP_FILE = "dump.bin";

static
void
usage(void)
{
    fprintf(stderr, "USAGE: %s [-d] [-m MEMSIZE] FILE\n", "bfjit");
    exit(2);
}

int main(int argc, char **argv)
{
    const char *err_msg = "mmap failed!\n";
    const size_t nerr_msg = strlen(err_msg);

    bool dump = false;
    unsigned nmem = 1 << 30;
    for (int c; (c = getopt(argc, argv, "dm:"))  != -1;) {
        switch (c) {
        case 'd': dump = true; break;
        case 'm': nmem = strtoul(optarg, NULL, 10); break;
        default: usage(); break;
        }
    }
    if (argc - optind != 1) {
        usage();
    }
    FILE *fsrc = fopen(argv[optind], "r");
    if (!fsrc) {
        perror(argv[optind]);
        return 1;
    }

    VECTOR_OF(size_t) stack = VECTOR_NEW();
    VECTOR_OF(size_t) fixup_write = VECTOR_NEW();
    VECTOR_OF(size_t) fixup_read = VECTOR_NEW();
    size_t fixup_error;

    ptr = mmap(NULL, capacity, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        abort();
    }

    i_movq_r_im(RAX, 9);                                            // mmap(
    i_movq_r_im(RDI, 0);                                            //  addr,
    i_movq_r_im(RSI, nmem);                                         //  length,
    i_movq_r_im(RDX, PROT_READ | PROT_WRITE);                       //  prot,
    i_movq_rn_im(R10, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE); //  flags,
    i_movq_rn_im(R8,  -1);                                          //  fd,
    i_movq_rn_im(R9,  0);                                           //  offset
    i_syscall();                                                    // )

    i_test_r_r(RAX, RAX);
    i_js(0);
    fixup_error = size;

    i_movq_r_r(RBX, RAX);

    for (int c; (c = getc(fsrc)) != EOF;) {
        switch (c) {
        case '>': i_incq_r(RBX); break;
        case '<': i_decq_r(RBX); break;
        case '+': i_incb_m(RBX); break;
        case '-': i_decb_m(RBX); break;
        case '[': i_cmpb_m_im(RBX, 0); i_je(0); VECTOR_PUSH(stack, size); break;
        case ']':
            {
                if (!stack.size) {
                    fputs("unmatched ']'.\n", stderr);
                    return 1;
                }
                const size_t p = VECTOR_POP(stack);
                const size_t s = size;
                i_jmp(p - 5 - 6 - 3 - s);
                fixup4(p);
            }
            break;
        case '.': i_call(0); VECTOR_PUSH(fixup_write, size); break;
        case ',': i_call(0); VECTOR_PUSH(fixup_read, size); break;
        }
    }

    if (stack.size) {
        fputs("unmatched '['.\n", stderr);
        return 1;
    }

    i_movq_r_im(RAX, 60); // exit(
    i_movq_r_im(RDI, 0);  //  status
    i_syscall();          // )

    fixup4(fixup_error);

    i_movq_r_im(RAX, 1);                        // write(
    i_movq_r_im(RDI, 2);                        //  fd,
    i_movq_r_im8(RSI, (unsigned long) err_msg); //  buffer,
    i_movq_r_im(RDX, nerr_msg);                 //  count
    i_syscall();                                // )

    i_movq_r_im(RAX, 60); // exit(
    i_movq_r_im(RDI, 1);  //  status
    i_syscall();          // )

    for (size_t i = 0; i < fixup_write.size; ++i) {
        fixup4(fixup_write.data[i]);
    }
    i_movq_r_im(RAX, 1);  // write(
    i_movq_r_im(RDI, 1);  //  fd,
    i_movq_r_r(RSI, RBX); //  buffer,
    i_movq_r_im(RDX, 1);  //  count
    i_syscall();          // )
    i_ret();

    for (size_t i = 0; i < fixup_read.size; ++i) {
        fixup4(fixup_read.data[i]);
    }
    i_movq_r_im(RAX, 0);  // read(
    i_movq_r_im(RDI, 0);  //  fd,
    i_movq_r_r(RSI, RBX); //  buffer,
    i_movq_r_im(RDX, 1);  //  count
    i_syscall();          // )

    // if (rax < 1) *(char*)rbx = 0;
    i_negq_r(RAX);
    i_shrq_r_im(RAX, 24);
    i_andb_m_r(RBX, RAX);

    i_ret();

    VECTOR_FREE(stack);
    VECTOR_FREE(fixup_write);
    VECTOR_FREE(fixup_read);

    if (dump) {
        FILE *fdump = fopen(DUMP_FILE, "w");
        if (!fdump) {
            perror(DUMP_FILE);
            return 1;
        }
        if (fwrite(ptr, 1, size, fdump) != size) {
            perror("cannot write dump file");
            return 1;
        }
        fclose(fdump);
        fprintf(stderr, "HINT: run\n\t objdump -D -b binary -m i386:x64-32:intel '%s'\n",
                DUMP_FILE);
        return 0;
    } else {
        void (*func)(void);
        *(void **) &func = ptr;
        func();
    }
}
