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
        if (errno != EAGAIN && errno != EINTR) break;
        usleep(1000);
    }
    return 0;
}
