#include "my_sscanf.h"
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "libls/parse_int.h"

int my_sscanf(const char *s, const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    int ret = MY_SSCANF_OK;
    for (;;) {
        if (*fmt == '%') {
            ++fmt;
            switch (*fmt) {
            case '%':
                if (*s != '%') {
                    ret = MY_SSCANF_CANTFOLLOW;
                    goto done;
                }
                ++fmt, ++s;
                break;
            case 'u':
                {
                    int r = ls_parse_uint_b(s, SIZE_MAX, &s);
                    if (r < 0) {
                        ret = MY_SSCANF_OVERFLOW;
                        goto done;
                    }
                    unsigned *ptr = va_arg(vl, unsigned *);
                    *ptr = r;
                }
                ++fmt;
                break;
            case 'w':
                {
                    size_t nbuf = va_arg(vl, size_t);
                    char *buf = va_arg(vl, char *);
                    const char *word_end = strchr(s, fmt[1]);
                    if (!word_end) {
                        ret = MY_SSCANF_CANTFOLLOW;
                        goto done;
                    }
                    size_t nword = word_end - s;
                    if (nword + 1 > nbuf) {
                        ret = MY_SSCANF_OVERFLOW;
                        goto done;
                    }
                    if (nword) {
                        memcpy(buf, s, nword);
                    }
                    buf[nword] = '\0';
                    s = word_end;
                    ++fmt;
                }
                break;
            default:
                ret = MY_SSCANF_INVLDFMT;
                goto done;
            }
        } else if (*fmt != *s) {
            ret = MY_SSCANF_CANTFOLLOW;
            goto done;
        } else if (!*fmt) {
            goto done;
        } else {
            ++fmt, ++s;
        }
    }
done:
    va_end(vl);
    return ret;
}
