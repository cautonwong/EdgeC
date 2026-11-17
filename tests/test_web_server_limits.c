/**
 * @file test_web_server_limits.c
 * @brief The web server rejects oversized and incomplete requests.
 */

#include <cmocka.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <uv.h>

#include "web_server.h"

static char *dummy_api(void *user_data, const char *path, const char *body) {
    (void)user_data;
    (void)path;
    (void)body;
    return strdup("{}");
}

static int connect_client(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert_true(fd >= 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    assert_int_equal(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr), 1);
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    assert_int_equal(connect(fd, (struct sockaddr *)&addr, sizeof(addr)), 0);
    return fd;
}

static int pump_response(uv_loop_t *loop, int fd, char *out, size_t cap) {
    size_t used = 0;
    out[0] = '\0';
    for (int i = 0; i < 2000; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        ssize_t n = recv(fd, out + used, cap - 1 - used, 0);
        if (n > 0) {
            used += (size_t)n;
            out[used] = '\0';
            if (strstr(out, "\r\n\r\n")) return 1;
            continue;
        }
        if (n == 0) break;
        /* A signal is not an answer: retry, the loop gets another turn. */
        if (errno != EAGAIN && errno != EINTR) break;
        usleep(1000);
    }
    return used > 0;
}

static void drain_loop(uv_loop_t *loop) {
    for (int i = 0; i < 100; i++) uv_run(loop, UV_RUN_NOWAIT);
}

static void test_oversized_request_rejected(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    web_server_t *srv = web_server_create(&loop, 0, ".");
    assert_non_null(srv);
    int port = web_server_port(srv);
    assert_true(port > 0);

    int fd = connect_client(port);
    char request[4096];
    memset(request, 'A', sizeof(request));
    const char *start = "GET / HTTP/1.1\r\n";
    memcpy(request, start, strlen(start));
    assert_int_equal(send(fd, request, sizeof(request), 0), (ssize_t)sizeof(request));

    char out[1024];
    assert_true(pump_response(&loop, fd, out, sizeof(out)));
    assert_true(strncmp(out, "HTTP/1.1 413", 12) == 0);

    close(fd);
    web_server_destroy(srv);
    drain_loop(&loop);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_incomplete_api_request_rejected(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    web_server_t *srv = web_server_create(&loop, 0, ".");
    assert_non_null(srv);
    web_server_set_api_handler(srv, dummy_api, NULL);
    int port = web_server_port(srv);
    assert_true(port > 0);

    int fd = connect_client(port);
    const char *request = "GET /api/x HTTP/1.1\r\nHost: x\r\n";
    assert_int_equal(send(fd, request, strlen(request), 0), (ssize_t)strlen(request));

    char out[1024];
    assert_true(pump_response(&loop, fd, out, sizeof(out)));
    assert_true(strncmp(out, "HTTP/1.1 400", 12) == 0);

    close(fd);
    web_server_destroy(srv);
    drain_loop(&loop);
    assert_int_equal(uv_loop_close(&loop), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_oversized_request_rejected),
        cmocka_unit_test(test_incomplete_api_request_rejected),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
