#include <errno.h>
#include <sys/socket.h>

static int try_read(int fd, char *out, unsigned cap) {
    struct timeval tv = {1, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ssize_t n = recv(fd, out, cap, MSG_DONTWAIT);
    if (n < 0 && errno == EAGAIN) return 0;
    return 1;
}
