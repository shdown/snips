#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int
main()
{
    int fd;

    struct sockaddr_nl sa;
    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;

    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0) {
        perror("socket");
        return 1;
    }
    if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        perror("bind");
        return 1;
    }

    // request an ACK
    {
        struct nlmsghdr *nh = malloc(sizeof(struct nlmsghdr));
        if (!nh) {
            fprintf(stderr, "Out of memory.\n");
            return 1;
        }
        *nh = (struct nlmsghdr) {
            .nlmsg_len = sizeof(struct nlmsghdr),
            .nlmsg_type = NLMSG_NOOP,
            .nlmsg_flags = NLM_F_ACK,
            .nlmsg_pid = 0,
            .nlmsg_seq = 1,
        };
        struct iovec iov = { nh, nh->nlmsg_len };
        struct sockaddr_nl msg_sa;
        struct msghdr msg = {&msg_sa, sizeof(msg_sa), &iov, 1, NULL, 0, 0};
        memset(&msg_sa, 0, sizeof(msg_sa));
        msg_sa.nl_family = AF_NETLINK;
        if (sendmsg(fd, &msg, 0) < 0) {
            perror("sendmsg");
            return 1;
        }
    }

    char buf[8192];     /* 8192 to avoid message truncation on platforms with page size > 4096 */
    struct iovec iov = {buf, sizeof(buf)};
    struct sockaddr_nl msg_sa;
    struct msghdr msg = {&msg_sa, sizeof(msg_sa), &iov, 1, NULL, 0, 0};

    while (1) {
        ssize_t len = recvmsg(fd, &msg, 0);
        if (len < 0) {
            perror("recvmsg");
            return 1;
        }
        fprintf(stderr, "Received %zd\n", len);
        for (struct nlmsghdr *nh = (struct nlmsghdr *) buf;
             NLMSG_OK(nh, len);
             nh = NLMSG_NEXT(nh, len))
        {
            /* The end of multipart message */
            if (nh->nlmsg_type == NLMSG_DONE) {
                break;
            }

            if (nh->nlmsg_type == NLMSG_ERROR) {
                struct nlmsgerr *e = (struct nlmsgerr *) (((char *) nh) + sizeof(struct nlmsghdr));
                int errnum = e->error;
                if (errnum) {
                    fprintf(stderr, "Error: %s\n", strerror(-errnum));
                    return 1;
                }
                fprintf(stderr, "ACK\n");
                continue;
            }

            fprintf(stderr, "Part\n");
            if (nh->nlmsg_flags & NLM_F_ACK) {
                fprintf(stderr, "ACK requested!\n");
            }
            if (nh->nlmsg_flags & NLM_F_ECHO) {
                fprintf(stderr, "Echo requested!\n");
            }
        }
    }
}
