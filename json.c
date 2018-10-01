#include <stddef.h>
#include <string.h>

typedef enum {
    JTT_MAP_START,
    JTT_MAP_END,

    JTT_LIST_START,
    JTT_LIST_END,

    JTT_STRING,
    JTT_NUMBER,
    JTT_TRUE,
    JTT_FALSE,
    JTT_NULL,

    JTT_ERROR,
    JTT_EOT,
} JsonTokenType;

JsonTokenType
json_advance(const char **buf, const char *end, const char **start)
{
    const char *s = *buf;

#define RET(Jtt_) *buf = s; return Jtt_

#define CHECK_INCR(N_) \
    if (end - s < N_) { RET(JTT_ERROR); } \
    s += N_

    static const unsigned char tableisnum[256] = {
        ['0' ... '9'] = 1,
        ['.'] = 1,
        ['-'] = 1,
        ['e'] = 1,
        ['E'] = 1,
        ['+'] = 1,
    };

    for (; s != end; ++s) {
        switch (*s) {
            case '{': ++s; RET(JTT_MAP_START);
            case '}': ++s; RET(JTT_MAP_END);
            case '[': ++s; RET(JTT_LIST_START);
            case ']': ++s; RET(JTT_LIST_END);

            case '"':
                if (start) { *start = s; }
                ++s;
                while (1) {
                    const size_t n = end - s;
                    if (!n || !(s = memchr(s, '"', n))) {
                        RET(JTT_ERROR);
                    }

                    size_t nbs = 1;
                    while (s[-nbs] == '\\') { ++nbs; }
                    --nbs;

                    ++s;
                    if (!(nbs & 1)) { RET(JTT_STRING); }
                }
                break;

            case '-':
            case '0' ... '9':
                if (start) { *start = s; }
                do {
                    ++s;
                } while (s != end && tableisnum[(unsigned char) *s]);
                RET(JTT_NUMBER);

            case 't': CHECK_INCR(4); RET(JTT_TRUE);
            case 'f': CHECK_INCR(5); RET(JTT_FALSE);
            case 'n': CHECK_INCR(4); RET(JTT_NULL);
        }
    }
    RET(JTT_EOT);
#undef CHECK_INCR
#undef RET
}

void
json_skip(const char **buf, const char *end)
{
    size_t level = 0;
    do {
        switch (json_advance(buf, end, NULL)) {
        case JTT_MAP_START:
        case JTT_LIST_START:
            ++level;
            break;
        case JTT_MAP_END:
        case JTT_LIST_END:
            --level;
            break;
        case JTT_EOT:
        case JTT_ERROR:
            return;
        default:
            break;
        }
    } while (level);
}
