#include "net.h"
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "libls/vector.h"
#include "libls/string_.h"
#include "libls/io_utils.h"
#include "common/span.h"
#include "../proto/proto.h"

#define Conn  net_Conn_
#define conns net_conns_

#if EAGAIN == EWOULDBLOCK
# define IS_EAGAIN(ErrNum_) ((ErrNum_) == EAGAIN)
#else
# define IS_EAGAIN(ErrNum_) ((ErrNum_) == EAGAIN || (ErrNum_) == EWOULDBLOCK)
#endif

// Our /pollfds/ vector will always have one extra leading element for the server socket.
// That means /conns.data[i]/ corresponds to /pollfds.data[i + 1]/.
enum { POLLFDS_OFFSET = 1 };

net_conns_vector_ conns = LS_VECTOR_NEW();
static LS_VECTOR_OF(struct pollfd) pollfds = LS_VECTOR_NEW();

static inline Conn conn_new(void) {
    return (Conn) {
        .wq = LS_VECTOR_NEW(),
        .wq_written = 0,
        .kick_wq_sz = -1,
        .garbage = false,
        .proto_data = proto_data_new(),
    };
}

static inline void conn_destroy(Conn *c) {
    LS_VECTOR_FREE(c->wq);
    proto_data_destroy(&c->proto_data);
}

static bool write_chunk(size_t index) {
    Conn *c = &conns.data[index];

    ssize_t w = write(pollfds.data[index + POLLFDS_OFFSET].fd,
                      c->wq.data + c->wq_written,
                      c->wq.size - c->wq_written);
    if (w < 0) {
        return IS_EAGAIN(errno);
    }
    c->wq_written += w;
    if (c->wq_written == c->wq.size) {
        if (c->kick_wq_sz >= 0) {
            // OK, we've done writing everything we wanted; now disconnect it.
            c->garbage = true;
        }
        c->wq.size = 0;
        c->wq_written = 0;
        pollfds.data[index + POLLFDS_OFFSET].events &= ~POLLOUT;
    }
    return true;
}

static bool read_chunk(size_t index) {
    Span buf = proto_get_read_buf(index);
    ssize_t r = read(pollfds.data[index + POLLFDS_OFFSET].fd, buf.data, buf.size);
    switch (r) {
    case -1:
        return IS_EAGAIN(errno);
    case 0:
        return !buf.size;
    default:
        proto_process_chunk(index, r);
        return true;
    }
}

static const short SOCKFD_EVENTS = POLLIN | POLLERR;
static const short CONNFD_EVENTS = POLLIN | POLLERR | POLLHUP;

static inline void destroy(size_t index) {
    conn_destroy(&conns.data[index]);

    int fd = pollfds.data[index + POLLFDS_OFFSET].fd;
    shutdown(fd, SHUT_RDWR);
    close(fd);

    if (!pollfds.data[0].events) {
        fprintf(stderr, "I: accepting new clients now.\n");
        pollfds.data[0].events = SOCKFD_EVENTS;
    }
}

static void collect_garbage(void) {
    size_t offset = 0;
    for (size_t i = 0; ; ++i) {
        while (i + offset < conns.size && conns.data[i + offset].garbage) {
            destroy(i + offset);
            ++offset;
        }
        if (i + offset >= conns.size) {
            break;
        }
        if (offset) {
            pollfds.data[i + POLLFDS_OFFSET] = pollfds.data[i + offset + POLLFDS_OFFSET];
            conns.data[i] = conns.data[i + offset];
        }
    }
    pollfds.size -= offset;
    conns.size   -= offset;
}

// Assumes /conns.data[i].garbage == false/.
static inline bool interact(size_t index) {
    short revents = pollfds.data[index + POLLFDS_OFFSET].revents;
    if (revents & POLLIN) {
        if (!read_chunk(index)) {
            return false;
        }
    }
    if (revents & POLLOUT) {
        if (!write_chunk(index)) {
            return false;
        }
    }
    if (revents & (POLLERR | POLLHUP)) {
        return false;
    }
    return true;
}

static int prepare_get_sockfd(uint16_t port, bool reuse_addr) {
    int sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        return -1;
    }
    if (reuse_addr) {
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
            return -1;
        }
    }
    struct sockaddr_in sa = {
        .sin_family = PF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    if (bind(sockfd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        return -1;
    }
    if (listen(sockfd, SOMAXCONN) < 0) {
        return -1;
    }
    if (ls_make_nonblock(sockfd) < 0) {
        return -1;
    }
    LS_VECTOR_PUSH(pollfds, ((struct pollfd) {.fd = sockfd, .events = SOCKFD_EVENTS}));
    return sockfd;
}

void net_run(uint16_t port, bool reuse_addr) {
    int sockfd = prepare_get_sockfd(port, reuse_addr);
    if (sockfd < 0) {
        return;
    }
    fprintf(stderr, "D: listening at port %d now.\n", (int) port);
    while (1) {
        if (poll(pollfds.data, pollfds.size, -1) < 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        if (pollfds.data[0].revents) {
            int fd;
            while ((fd = accept(sockfd, NULL, NULL)) >= 0) {
                LS_VECTOR_PUSH(pollfds, ((struct pollfd) {.fd = fd, .events = CONNFD_EVENTS}));
                LS_VECTOR_PUSH(conns, conn_new());
            }
            if (!IS_EAGAIN(errno)) {
                if (errno == EMFILE) {
                    fprintf(stderr, "E: EMFILE, not accepting new connections temporarily.\n");
                    fprintf(stderr, "I: try `ulimit -n <...>' or use `iptables' to filter out malicious clients.\n");
                    pollfds.data[0].events = 0;
                } else {
                    return;
                }
            }
        }
        for (size_t i = 0; i < conns.size; ++i) {
            if (conns.data[i].garbage) {
                continue;
            }
            if (!interact(i)) {
                conns.data[i].garbage = true;
                proto_bailed_out(i);
            }
        }
        collect_garbage();
    }
}

void net_conn_write_finish(size_t index) {
    Conn *c = &conns.data[index];

    if (c->kick_wq_sz >= 0) {
        c->wq.size = c->kick_wq_sz;
    }
    pollfds.data[index + POLLFDS_OFFSET].events |= POLLOUT;
}

void net_kick(size_t index) {
    Conn *c = &conns.data[index];

    if (c->kick_wq_sz < 0) {
        c->kick_wq_sz = c->wq.size;
    }
    pollfds.data[index + POLLFDS_OFFSET].events |= POLLOUT;
}
