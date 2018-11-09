#include <stdio.h>
#include <stdint.h>

void
fwritebmp(FILE *out, uint16_t width, uint16_t height, size_t ndata, const void *data)
{
    const unsigned bits_per_pixel = ndata * 8 / width / height;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#   define write2(x) fwrite((uint16_t [1]) {x}, 1, 2, out)
#   define write4(x) fwrite((uint32_t [1]) {x}, 1, 4, out)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#   define write2(x) fwrite((uint16_t [1]) {__builtin_bswap16(x)}, 1, 2, out)
#   define write4(x) fwrite((uint32_t [1]) {__builtin_bswap32(x)}, 1, 4, out)
#else
#   error "Here's a nickel kid. Go buy yourself a real computer."
#endif
    /* 0  */ fputs("BM", out);       /* signature */
    /* 2  */ write4(ndata + 26);     /* file size */
    /* 6  */ write4(0);              /* reserved */
    /* 10 */ write4(26);             /* pixels data offset */
    /* 14 */ write4(12);             /* following header size */
    /* 18 */ write2(width);          /* width */
    /* 20 */ write2(height);         /* height */
    /* 22 */ write2(1);              /* number of planes (reserved) */
    /* 24 */ write2(bits_per_pixel); /* bits per pixel */
    /* 26 */ fwrite(data, ndata, 1, out);
#undef write2
#undef write4
}
