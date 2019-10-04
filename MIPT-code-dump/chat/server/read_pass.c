#include "read_pass.h"

#include <stdlib.h>
#include <stdio.h>
#ifdef USE_TERMIOS
# include <termios.h>
#endif

char * read_pass(const char *prompt)
{
#ifdef USE_TERMIOS
    struct termios t;

    if (tcgetattr(0, &t) < 0) {
        perror("tcgetattr (stdin)");
        return NULL;
    }

    const tcflag_t t_old_c_lflag = t.c_lflag;
    t.c_lflag &= ~(ISIG | ECHO);
    if (tcsetattr(0, TCSAFLUSH, &t) < 0) {
        perror("tcsetattr (stdin)");
        return NULL;
    }
#endif

    if (prompt) {
        fputs(prompt, stderr);
    }
    char *buf = NULL;
    size_t nbuf = 0;
    ssize_t r = getline(&buf, &nbuf, stdin);
    if (r <= 0) {
        if (feof(stdin)) {
            fputs("\nGot end-of-file.\n", stderr);
        } else {
            perror("getline");
        }
        free(buf);
        buf = NULL;
    } else {
        if (buf[r - 1] == '\n') {
            buf[r - 1] = '\0';
        }
#ifdef USE_TERMIOS
        putc('\n', stderr);
#endif
    }

#ifdef USE_TERMIOS
    t.c_lflag = t_old_c_lflag;
    if (tcsetattr(0, TCSAFLUSH, &t) < 0) {
        perror("tcsetattr (stdin)");
    }
#endif

    return buf;
}
