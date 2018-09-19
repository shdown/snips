#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

void *
xmalloc(size_t n)
{
    void *r = malloc(n);
    if (n && !r) {
        fputs("Out of memory.\n", stderr);
        abort();
    }
    return r;
}

void *
xmalloc2(size_t n, size_t m)
{
    if (m && n > SIZE_MAX / m) {
        fputs("Insane amount of memory requested.\n", stderr);
        abort();
    }
    return xmalloc(n * m);
}

void *
xcalloc(size_t n, size_t m)
{
    void *r = calloc(n, m);
    if (n && m && !r) {
        fputs("Out of memory.\n", stderr);
        abort();
    }
    return r;
}

#define MAX(A_, B_) ((A_) > (B_) ? (A_) : (B_))

typedef unsigned long UWORD;

typedef struct {
    UWORD *words;
    size_t size;
} Number;

#define number_nonzero(Num_) (!!(Num_).size)

Number
number_new(void)
{
    return (Number) {NULL, 0};
}

Number
number_new_from_l(UWORD w)
{
    if (!w) {
        return (Number) {NULL, 0};
    }
    UWORD *words = xmalloc(sizeof(UWORD));
    words[0] = w;
    return (Number) {words, 1};
}

Number
number_copy(Number a)
{
    UWORD *words = xmalloc2(a.size, sizeof(UWORD));
    if (a.size) {
        memcpy(words, a.words, a.size * sizeof(UWORD));
    }
    return (Number) {words, a.size};
}

Number
number_add(Number a, Number b)
{
    if (!a.size) {
        return number_copy(b);
    }
    if (!b.size) {
        return number_copy(a);
    }
    unsigned size = MAX(a.size, b.size) + 1;
    UWORD *words = xcalloc(size, sizeof(UWORD));
    memcpy(words, a.words, a.size * sizeof(UWORD));

    __asm__(
        "mov %[Size], %%rcx\n"
        "xor %%r8, %%r8\n"

        "mov (%[Src]), %%r9\n"
        "add %%r9, (%[Dst])\n"

        "jmp loop_ctnue\n"
        "loop_begin:\n"
        "mov (%[Src],%%r8,8), %%r9\n"
        "adcq %%r9, (%[Dst],%%r8,8)\n"
        "loop_ctnue:\n"
        "inc %%r8\n"
        "loop loop_begin\n"

        "adcq $0, (%[Dst],%%r8,8)\n"

        : /* no outputs */

        : [Size] "r" (b.size)
        , [Dst] "r" (words)
        , [Src] "r" (b.words)

        : "cc", "memory", "r8", "r9", "rcx"
    );

    if (size && !words[size - 1]) {
        --size;
    }
    return (Number) {words, size};
}

int
number_cmp(Number a, Number b)
{
    if (a.size != b.size) {
        return a.size > b.size ? 1 : -1;
    }
    if (!a.size) {
        return 0;
    }

    int result;

    __asm__(
        "xor %%rcx, %%rcx\n"
        "z_loop:\n"
        "mov (%[BufB],%%rcx,8), %%rbx\n"
        "cmpq %%rbx, (%[BufA],%%rcx,8)\n"
        "jne differ\n"
        "add $1, %%rcx\n"
        "cmpq %%rcx, %[Size]\n"
        "jne z_loop\n"

        "xor %[Result], %[Result]\n"
        "jmp z_done\n"

        "differ:\n"
        "jg greater\n"
        "mov $-1, %[Result]\n"
        "jmp z_done\n"
        "greater:\n"
        "mov $1, %[Result]\n"

        "z_done:\n"

        : [Result] "=r" (result)

        : [BufA] "r" (a.words)
        , [BufB] "r" (b.words)
        , [Size] "r" (a.size)

        : "cc", "memory", "rcx", "rbx"
    );
    return result;
}

// assumes a >= b
Number
number_sub(Number a, Number b)
{
    Number c = number_copy(a);
    if (!a.size || !b.size) {
        return c;
    }

    __asm__(
        "mov %[SizeB], %%rcx\n"
        "xor %%r8, %%r8\n"

        "mov (%[BufB]), %%r9\n"
        "sub %%r9, (%[BufC])\n"

        "jmp y_loop_ctnue\n"
        "y_loop_begin:\n"
        "mov (%[BufB],%%r8,8), %%r9\n"
        "sbbq %%r9, (%[BufC],%%r8,8)\n"
        "y_loop_ctnue:\n"
        "inc %%r8\n"
        "loop y_loop_begin\n"

        "jnc y_done\n"
        "sub $1, (%[BufC],%%r8,8)\n"
        "y_done:\n"

        "y_norm:\n"
        "sub $1, %[SizeC]\n"
        "jc y_norm_done\n"
        "cmpq $0, (%[BufC],%[SizeC],8)\n"
        "je y_norm\n"
        "y_norm_done:\n"
        "add $1, %[SizeC]\n"

        : [SizeC] "+r" (c.size)

        : [SizeB] "r" (b.size)
        , [BufB] "r" (b.words)
        , [BufC] "r" (c.words)

        : "cc", "memory", "r8", "r9", "rcx"
    );

    return c;
}

static inline
unsigned long
fastpow_u64(unsigned long base, unsigned char exponent)
{
    unsigned long result = 1;
    for (; exponent; exponent >>= 1) {
        if (exponent & 1) {
            result *= base;
        }
        base *= base;
    }
    return result;
}

void
number_print(Number a, unsigned radix)
{
    static const char *const table = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    // digits per word: dpw[i] = floor(log_{i}(2^{64} - 1))
    static const unsigned char dpw[] = {
        0, 0, 63, 40, 31, 27, 24, 22, 21, 20, 19, 18, 17, 17, 16, 16, 15, 15, 15, 15, 14, 14, 14,
        14, 13, 13, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12, 12,
    };
    if (!a.size) {
        fputs("0\n", stdout);
        return;
    }

    const unsigned char exponent = dpw[radix];

    UWORD *buf = xmalloc2(a.size, sizeof(UWORD));
    memcpy(buf, a.words, a.size * sizeof(UWORD));

    char *res = xmalloc2(a.size + 1, exponent + 1);
    char *tail = res;

    unsigned long temp_size = a.size;
    __asm__(
        "sub $8, %[Buf]\n"

        "outer:\n"
        "mov %[Size], %%rcx\n"
        "xor %%rdx, %%rdx\n"

        "divpow:\n"
        "mov (%[Buf],%%rcx,8), %%rax\n"
        "div %[Pow]\n"
        "mov %%rax, (%[Buf],%%rcx,8)\n"
        "loop divpow\n"

        "mov %%rdx, %%rax\n"
        "mov %[Dpw], %%rcx\n"

        "divradix:\n"
        "xor %%rdx, %%rdx\n"
        "div %[Radix]\n"
        "mov (%[Table],%%rdx), %%dl\n"
        "mov %%dl, (%[Tail])\n"
        "add $1, %[Tail]\n"
        "loop divradix\n"

        "cmpq $0, (%[Buf],%[Size],8)\n"
        "jne outer\n"
        "sub $1, %[Size]\n"
        "jnz outer\n"

        "add $8, %[Buf]\n"

        : [Tail] "+r" (tail)
        , [Buf] "+r" (buf)
        , [Size] "+r" (temp_size)

        : [Table] "r" (table)
        , [Radix] "r" ((unsigned long) radix)
        , [Dpw] "r" ((unsigned long) exponent)
        , [Pow] "r" (fastpow_u64(radix, exponent))

        : "cc", "memory", "rax", "rcx", "rdx"
    );

    free(buf);

    const size_t nres = tail - res;
    for (size_t i = 0; i < nres / 2; ++i) {
        char tmp = res[i];
        res[i] = res[nres - i - 1];
        res[nres - i - 1] = tmp;
    }
    size_t start = 0;
    while (res[start] == '0') {
        ++start;
    }
    res[nres] = '\n';
    fwrite(res + start, 1, nres + 1 - start, stdout);

    free(res);
}

static
size_t
calc_ndigits(unsigned radix, size_t n)
{
    return (n / 64 + 1) * (32 - __builtin_clz(radix));
}

Number
number_parse(const char *s, size_t ns, unsigned radix)
{
    static const unsigned char table[] = {
        ['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4, ['5'] = 5, ['6'] = 6, ['7'] = 7,
        ['8'] = 8, ['9'] = 9,

        ['A'] = 10, ['B'] = 11, ['C'] = 12, ['D'] = 13, ['E'] = 14, ['F'] = 15, ['G'] = 16,
        ['H'] = 17, ['I'] = 18, ['J'] = 19, ['K'] = 20, ['L'] = 21, ['M'] = 22, ['N'] = 23,
        ['O'] = 24, ['P'] = 25, ['Q'] = 26, ['R'] = 27, ['S'] = 28, ['T'] = 29, ['U'] = 30,
        ['V'] = 31, ['W'] = 32, ['X'] = 33, ['Y'] = 34, ['Z'] = 25,

        ['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15, ['g'] = 16,
        ['h'] = 17, ['i'] = 18, ['j'] = 19, ['k'] = 20, ['l'] = 21, ['m'] = 22, ['n'] = 23,
        ['o'] = 24, ['p'] = 25, ['q'] = 26, ['r'] = 27, ['s'] = 28, ['t'] = 29, ['u'] = 30,
        ['v'] = 31, ['w'] = 32, ['x'] = 33, ['y'] = 34, ['z'] = 25,
    };
    const char *end = s + ns;

    const char *ptr = s;
    while (1) {
        if (ptr == end) {
            return (Number) {NULL, 0};
        }
        if (table[(unsigned char) *ptr]) {
            break;
        }
        ++ptr;
    }

    UWORD *words = xmalloc2(calc_ndigits(radix, end - ptr), sizeof(UWORD));
    words[0] = table[(unsigned char) *ptr];
    size_t size = 1;
    __asm__(
        "jmp x_outer_ctnue\n"

        "x_outer_begin:\n"
        "movzbq (%[Ptr]), %%rdx\n"
        "movzbq (%[Table],%%rdx), %%rdx\n"

        "xor %%rcx, %%rcx\n"

        "x_inner:\n"
        "mov (%[Buf],%%rcx,8), %%rax\n"
        "mov %%rdx, %%r10\n"
        "mul %[Radix]\n"
        "add %%r10, %%rax\n"
        "adc $0, %%rdx\n"
        "mov %%rax, (%[Buf],%%rcx,8)\n"
        "add $1, %%rcx\n"
        "cmp %%rcx, %[Size]\n"
        "jne x_inner\n"

        "mov %%rdx, (%[Buf],%%rcx,8)\n"
        "test %%rdx, %%rdx\n"
        "jz x_outer_ctnue\n"
        "add $1, %[Size]\n"

        "x_outer_ctnue:\n"
        "add $1, %[Ptr]\n"
        "cmp %[Ptr], %[End]\n"
        "jne x_outer_begin\n"

        : [Size] "+r" (size)
        , [Ptr] "+r" (ptr)

        : [Radix] "r" ((unsigned long) radix)
        , [Buf] "r" (words)
        , [End] "r" (end)
        , [Table] "r" (table)

        : "cc", "memory", "rax", "rcx", "rdx", "r10"
    );

    return (Number) {words, size};
}

void
number_dump(Number a)
{
    for (size_t i = 0; i < a.size; ++i) {
        printf(" | %lu", a.words[i]);
    }
    printf("\n");
}

void
number_free(Number a)
{
    free(a.words);
}

Number
read_num(void)
{
    char s[100];
    fgets(s, sizeof(s), stdin);
    size_t n = strlen(s);
    if (n && s[n - 1] == '\n') {
        --n;
    }
    return number_parse(s, n, 10);
}

int
main()
{
    //number_dump(a);
    Number a = read_num();
    Number b = read_num();
    printf("%d\n", number_cmp(a, b));
    //number_print(a, 2);
}
