#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "edge_fd_ipc.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "edge_log.h"

int edge_fd_send(int sock_fd, int fd_to_send) {
    struct msghdr msg = {0};
    char buf[CMSG_SPACE(sizeof(int))];
    memset(buf, 0, sizeof(buf));

    struct iovec io = {.iov_base = (void *)"FD", .iov_len = 2};

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));

    *((int *)CMSG_DATA(cmsg)) = fd_to_send;

    if (sendmsg(sock_fd, &msg, 0) < 0) {
        LOG_ERROR(LOG_CAT_BUS, "edge_fd_send failed: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int edge_fd_recv(int sock_fd) {
    struct msghdr msg = {0};
    char buf[CMSG_SPACE(sizeof(int))];
    struct iovec io = {.iov_base = buf, .iov_len = sizeof(buf)};

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    if (recvmsg(sock_fd, &msg, 0) < 0) {
        LOG_ERROR(LOG_CAT_BUS, "edge_fd_recv failed: %s", strerror(errno));
        return -1;
    }

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
        return *((int *)CMSG_DATA(cmsg));
    }
    return -1;
}
