#include "net.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include "libls/errno_utils.h"

int tcp_open(const char *hostname, const char *service, FILE *err) {
    struct addrinfo *ai = NULL;
    int fd = -1;

    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = 0,
        .ai_flags = AI_ADDRCONFIG,
    };

    int gai_r = getaddrinfo(hostname, service, &hints, &ai);
    if (gai_r) {
        if (gai_r == EAI_SYSTEM) {
            LS_WITH_ERRSTR(s, errno,
                fprintf(err, "getaddrinfo: %s\n", s);
            );
        } else {
            fprintf(err, "getaddrinfo: %s\n", gai_strerror(gai_r));
        }
        ai = NULL;
        goto cleanup;
    }

    for (struct addrinfo *pai = ai; pai; pai = pai->ai_next) {
        if ((fd = socket(pai->ai_family, pai->ai_socktype, pai->ai_protocol)) < 0) {
            LS_WITH_ERRSTR(s, errno,
                fprintf(err, "(candidate) socket: %s\n", s);
            );
            continue;
        }
        if (connect(fd, pai->ai_addr, pai->ai_addrlen) < 0) {
            LS_WITH_ERRSTR(s, errno,
                fprintf(err, "(candidate) connect: %s\n", s);
            );
            close(fd);
            fd = -1;
            continue;
        }
        break;
    }
    if (fd < 0) {
        fprintf(err, "can't connect to any of the candidates\n");
    }

cleanup:
    if (ai) {
        freeaddrinfo(ai);
    }
    return fd;
}
