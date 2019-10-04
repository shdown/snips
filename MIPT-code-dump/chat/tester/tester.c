#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "libls/io_utils.h"

static struct addrinfo *resolved;

static void resolve(const char *hostname, const char *service) {
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = 0,
        .ai_flags = AI_ADDRCONFIG,
    };
    int gai_r = getaddrinfo(hostname, service, &hints, &resolved);
    if (gai_r) {
        if (gai_r == EAI_SYSTEM) {
            perror("getaddrinfo");
        } else {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai_r));
        }
        exit(1);
    }
}

static int resolved_connect(void) {
    int fd = -1;
    for (struct addrinfo *pai = resolved; pai; pai = pai->ai_next) {
        if ((fd = socket(pai->ai_family, pai->ai_socktype, pai->ai_protocol)) < 0) {
            perror("(candidate) socket");
            continue;
        }
        if (connect(fd, pai->ai_addr, pai->ai_addrlen) < 0) {
            perror("(candidate) connect");
            close(fd);
            fd = -1;
            continue;
        }
        break;
    }
    if (fd < 0) {
        fprintf(stderr, "can't connect to any of the candidates\n");
    }
    return fd;
}

static void test_dos(void) {
    for (int i = 0; ; ++i) {
        fprintf(stderr, "i = %7d\n", i);
        if (resolved_connect() < 0) {
            return;
        }
    }
}

static void test_bigdata(void) {
    for (int i = 0; ; ++i) {
        fprintf(stderr, "i = %7d\n", i);
        int fd = resolved_connect();
        if (fd < 0) {
            return;
        }
        if (ls_make_nonblock(fd) < 0) {
            perror("ls_make_nonblock");
            return;
        }
        if (write(fd, (unsigned char [6]) {'i', 0xff, 0xff, 0xff, 0xff, 'x'}, 6) < 0) {
            perror("write");
            return;
        }
    }
}

static void usage(void) {
    fprintf(stderr, "USAGE: tester <host> <service> dos\n");
    fprintf(stderr, "       tester <host> <service> bigdata\n");
    exit(2);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        usage();
    }
    if (strcmp(argv[3], "dos") == 0) {
        resolve(argv[1], argv[2]);
        test_dos();
    } else if (strcmp(argv[3], "bigdata") == 0) {
        resolve(argv[1], argv[2]);
        test_bigdata();
    } else {
        usage();
    }
    fprintf(stderr, "\n-------- hanging up -------\n");
    while (1) {
        pause();
    }
}
