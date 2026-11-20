#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

static int pump_response(int fd, char *out, unsigned cap) {
    struct timeval tv = {1, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    for (;;) {
        ssize_t n = recv(fd, out, cap, 0);
        if (n > 0) return 1;
        if (n == 0) break;
        /* EINTR 只在注释里,不算处理 */
        if (errno != EAGAIN) break;
        usleep(1000);
    }
    return 0;
}
