// TCP echo server using edge-triggered epoll.
// Written for https://www.linux.org.ru/forum/development/13538162/.
#include <sys/epoll.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>

static void * xmalloc(size_t nelems, size_t elemsz)
{
    size_t n = nelems * elemsz;
    void *r = malloc(n);
    if (!r && n) {
        fprintf(stderr, "Out of memory.\n");
        abort();
    }
    return r;
}

static int make_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return -1;
    }
    return 0;
}

//-----------------------------------------------------------------------------

enum { BUFSZ = 4 * 1024 };
typedef struct {
    union {
        char buf[BUFSZ];
        void *ptr;
    } u;
    size_t buf_start, buf_end;
    int fd;
} Datum;

#define datum_ptr(Dtm_) (Dtm_)->u.ptr

//-----------------------------------------------------------------------------
// slot allocator

static Datum *sal_trash_top;
static size_t sal_next_alloc = 1024; // must be >= 2

static Datum * sal_alloc(void)
{
    if (sal_trash_top) {
        Datum *r = sal_trash_top;
        sal_trash_top = datum_ptr(sal_trash_top);
        return r;
    } else {
        Datum *r = xmalloc(sal_next_alloc, sizeof(Datum));
        for (size_t i = 1; i < sal_next_alloc - 1; ++i) {
            datum_ptr(r + i) = &r[i + 1];
        }
        datum_ptr(r + sal_next_alloc - 1) = NULL;
        sal_trash_top = r + 1;
        sal_next_alloc *= 2;
        return r;
    }
}

static void sal_free(Datum *p)
{
    datum_ptr(p) = sal_trash_top;
    sal_trash_top = p;
}

//-----------------------------------------------------------------------------

static Datum * datum_create(int fd)
{
    Datum *d = sal_alloc();
    d->buf_start = d->buf_end = 0;
    d->fd = fd;
    return d;
}

static void datum_destroy(Datum *d)
{
    close(d->fd);
    sal_free(d);
}

#define datum_fd(Dtm_) (Dtm_)->fd

static inline bool echo(Datum *d)
{
    int fd = d->fd;
    char *buf = d->u.buf;

    size_t buf_start = d->buf_start;
    size_t buf_end   = d->buf_end;
    while (buf_start != buf_end) {
        ssize_t w = write(fd, buf + buf_start, buf_end - buf_start);
        if (w < 0) {
            return errno == EAGAIN;
        }
        d->buf_start = (buf_start += w);
    }

    while (1) {
        ssize_t r = read(fd, buf, BUFSZ);
        switch (r) {
        case -1:
            return errno == EAGAIN;
        case 0:
            return false;
        default:
            for (size_t written = 0; written != (size_t) r;) {
                ssize_t w = write(fd, buf + written, r - written);
                if (w < 0) {
                    d->buf_start = written;
                    d->buf_end = r;
                    return errno == EAGAIN;
                }
                written += w;
            }
        }
    }

    return true;
}

//-----------------------------------------------------------------------------

enum { EPEVBUF_SIZE = 1024 };

void srv_epoll_run(int sfd)
{
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        return;
    }
    struct epoll_event *epevbuf = xmalloc(EPEVBUF_SIZE, sizeof(struct epoll_event));
    int efd = epoll_create1(0);
    if (efd < 0) {
        perror("epoll_create1");
        return;
    }
    if (make_nonblock(sfd) < 0) {
        perror("make_nonblock (on server socket fd)");
        return;
    }
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &(struct epoll_event) {
            .events = EPOLLIN | EPOLLET,
            .data.ptr = NULL,
        }) < 0)
    {
        perror("epoll_ctl (EPOLL_CTL_ADD on server socket fd)");
        return;
    }
    while (1) {
        int nfds = epoll_wait(efd, epevbuf, EPEVBUF_SIZE, -1);
        if (nfds < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            return;
        }
        for (int i = 0; i < nfds; ++i) {
            Datum *datum = epevbuf[i].data.ptr;
            if (datum) {
                if (!echo(datum)) {
                    if (epoll_ctl(efd, EPOLL_CTL_DEL, datum_fd(datum), NULL) < 0) {
                        perror("epoll_ctl (EPOLL_CTL_DEL on client fd)");
                        return;
                    }
                    datum_destroy(datum);
                }
            } else {
                while (1) {
                    int cfd = accept(sfd, NULL, NULL);
                    if (cfd < 0) {
                        break;
                    }
                    if (make_nonblock(cfd) < 0) {
                        perror("make_nonblock (on client fd)");
                        return;
                    }
                    if (epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &(struct epoll_event) {
                            .events = EPOLLIN | EPOLLOUT | EPOLLET,
                            .data.ptr = datum_create(cfd),
                        }) < 0)
                    {
                        perror("epoll_ctl (EPOLL_CTL_ADD on client fd)");
                        return;
                    }
                }
                if (errno != EAGAIN) {
                    perror("accept");
                    return;
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------

static void usage(void)
{
    fprintf(stderr, "USAGE: srv_epoll PORT\n");
    exit(2);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage();
    }
    errno = 0;
    char *endptr;
    unsigned long port = strtoul(argv[1], &endptr, 10);
    if (errno || endptr == argv[1] || *endptr != '\0') {
        fprintf(stderr, "PORT is not an integer.\n");
        usage();
    }
    if (port == 0 || port > 65535) {
        fprintf(stderr, "PORT is not a valid port number.\n");
        usage();
    }

    int sfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sfd < 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_in sa = {
        .sin_family = PF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    if (bind(sfd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        perror("bind");
        return 1;
    }
    if (listen(sfd, SOMAXCONN) < 0) {
        perror("listen");
        return 1;
    }
    srv_epoll_run(sfd);
    return 1;
}
