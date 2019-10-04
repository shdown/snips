#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "net/net.h"
#include "proto/proto.h"
#include "chat/chat.h"
#include "libls/parse_int.h"
#include "read_pass.h"

enum { DEFAULT_PORT = 1337 };

static void usage(void) {
    fprintf(stderr,
"USAGE: server [-r] [<port>]\n"
"Default port is %d.\n"
"    -r: reuse port\n",
    (int) DEFAULT_PORT);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    int port = DEFAULT_PORT;
    bool reuse_addr = false;
    for (int c; (c = getopt(argc, argv, "r")) != -1;) {
        switch (c) {
        case 'r':
            reuse_addr = true;
            break;
        default:
            usage();
        }
    }
    if (optind != argc) {
        if (argc - optind > 1) {
            usage();
        }
        port = ls_full_parse_uint_s(argv[optind]);
        if (port < 0) {
            fprintf(stderr, "<port> is not a proper integer.\n");
            usage();
        } else if (port < 1 || port > 65535) {
            fprintf(stderr, "<port> is not a valid port number.\n");
            usage();
        }
    }

    char *root_pass = read_pass("Chatroom's root password: ");
    if (!root_pass) {
        return EXIT_FAILURE;
    }
    chat_init(root_pass);
    free(root_pass);

    proto_init();

    net_run(port, reuse_addr);
    perror("server");

    return EXIT_FAILURE;
}
